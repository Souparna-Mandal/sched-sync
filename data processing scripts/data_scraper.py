#/usr/bin/python3
import subprocess
import matplotlib.pyplot as plt

def run_subversion_demo_with_timeout(nthreads=3, duration=2, cs=(100,1,10), timeout_sec=5):
    """
    Runs the subversion_demo program with the given arguments and enforces a timeout.
    If the program does not finish within 'timeout_sec' seconds, it is killed, and
    any partial output captured so far is returned and parsed.

    :param nthreads:    Number of threads (example default: 3)
    :param param2:      Example parameter (example default: 2)
    :param param3:      Example parameter (example default: 1)
    :param param4:      Example parameter (example default: 1000)
    :param param5:      Example parameter (example default: 1)
    :param timeout_sec: Kill the process after this many seconds (default: 12)
    :return: A list of dictionaries with fields:
             {
               'thread_id': <string>,
               'loop_no': <int>,
               'lock_acquires': <int>,
               'lock_hold_us': <int>
             }
    """
    
    cmd = [
        "./bin/subversion_demo",
        str(nthreads),
        str(duration),
    ]
    
    for i in cs:
        cmd.append(str(i))

    try:
        result = subprocess.run(
            cmd, 
            capture_output=True, 
            text=True, 
            timeout=timeout_sec
        )
        output = result.stdout
    except subprocess.TimeoutExpired as e:
        output = e.output or ""

    results = []
    for line in output.splitlines():
        line = line.strip()
        # Expect lines of the form:
        # "id 00 loop 26460939 lock_acquires 662235 lock_hold(us) 665367"
        if line.startswith("id "):
            parts = line.split()
            if len(parts) == 8:
                # parts: ["id", thread_id, "loop", loop_no, "lock_acquires",
                #         lock_acquires, "lock_hold(us)", lock_hold_us]
                thread_id     = parts[1]
                loop_no       = int(parts[3])
                lock_acquires = int(parts[5])
                lock_hold_us  = int(parts[7])
                
                results.append({
                    "thread_id": thread_id,
                    "loop_no": loop_no,
                    "lock_acquires": lock_acquires,
                    "lock_hold_us": lock_hold_us
                })

    return results


def duration_experiments():
    """
    Runs subversion_demo for different durations and computes ratios of
    (thread 0 / thread 1) for lock_hold_us, lock_acquires, and loop_no.
    Then plots these ratios vs. the duration using matplotlib.

    :return: A dictionary mapping duration -> {
        "ratio_lock_hold_us": float or None,
        "ratio_lock_acquires": float or None,
        "ratio_loop_no": float or None
    }
    """
    # durations = [1, 2, 3, 4, 5, 10, 100, 1000]
    durations = [1, 2, 5, 10, 20]

    results = {}
    ratio_hold_list = []
    ratio_acquires_list = []
    ratio_loop_list = []

    for dur in durations:
        run_data = run_subversion_demo_with_timeout(
            nthreads=2,
            duration=dur,
            cs=(1, 10),
            timeout_sec=30
        )
        print(" Duration (s):" , dur)
        for record in run_data:
            print(record)

        t0 = None
        t1 = None
        for record in run_data:
            if record["thread_id"] == "00":
                t0 = record
            elif record["thread_id"] == "01":
                t1 = record

        if t0 and t1:
            ratio_lock_hold_us = (
                t0["lock_hold_us"] / t1["lock_hold_us"]
                if t1["lock_hold_us"] != 0 else None
            )
            ratio_lock_acquires = (
                t0["lock_acquires"] / t1["lock_acquires"]
                if t1["lock_acquires"] != 0 else None
            )
            ratio_loop_no = (
                t0["loop_no"] / t1["loop_no"]
                if t1["loop_no"] != 0 else None
            )
        else:
            ratio_lock_hold_us = None
            ratio_lock_acquires = None
            ratio_loop_no = None

        results[dur] = {
            "ratio_lock_hold_us": ratio_lock_hold_us,
            "ratio_lock_acquires": ratio_lock_acquires,
            "ratio_loop_no": ratio_loop_no
        }

        ratio_hold_list.append(ratio_lock_hold_us if ratio_lock_hold_us else 0)
        ratio_acquires_list.append(ratio_lock_acquires if ratio_lock_acquires else 0)
        ratio_loop_list.append(ratio_loop_no if ratio_loop_no else 0)

    # Create a figure with 3 subplots, side by side
    fig, axs = plt.subplots(1, 3, figsize=(15, 4))

    # 1) Lock Hold Ratio
    axs[0].plot(durations, ratio_hold_list, marker="o", color="blue")
    axs[0].set_xlabel("Duration")
    axs[0].set_ylabel("Lock Hold Time Ratio (T0/T1)")
    axs[0].set_title("Lock Hold Ratio vs. Duration")

    # 2) Lock Acquire Ratio
    axs[1].plot(durations, ratio_acquires_list, marker="o", color="green")
    axs[1].set_xlabel("Duration")
    axs[1].set_ylabel("Lock Acquire Ratio (T0/T1)")
    axs[1].set_title("No. of Lock Acquires Ratio vs. Duration")

    # 3) Loop No Ratio
    axs[2].plot(durations, ratio_loop_list, marker="o", color="red")
    axs[2].set_xlabel("Duration")
    axs[2].set_ylabel("Loop No. Ratio (T0/T1)")
    axs[2].set_title("Loop No. Ratio vs. Duration")

    plt.tight_layout()
    
    # Save the plot to a file instead of showing it
    plt.savefig("experiment_results.png", dpi=150)
    plt.close(fig)  # Close the figure to free memory

    return results
    

if __name__ == "__main__":
    # Example usage:
    # data = run_subversion_demo_with_timeout()
    # print("got data", data)
    # for record in data:
    #     print(record)
    print( duration_experiments())
