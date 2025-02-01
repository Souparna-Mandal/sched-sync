import matplotlib.pyplot as plt
import numpy as np

# Critical-section ratios
ratios = [1, 2, 3, 5, 10]

# --------------------- Loop Counts --------------------- #
loop_id00 = [213210165, 202178527, 209902135, 219976974, 226743221]
loop_id01 = [210984911, 246575002, 252697022, 257537536, 258866192]

# ------------------ Lock Acquires ---------------------- #
acq_id00  = [4822807,   4841693,   4842466,   4825796,   4874298]
acq_id01  = [4822788,   2432593,   1609995,   969544,    486718]

# ------------------- Lock Hold (us) -------------------- #
hold_id00 = [4961593,   4948599,   4955282,   4959475,   4973242]
hold_id01 = [4961387,   4994102,   4995550,   4997435,   4998862]

# Create a figure with 3 subplots, side by side
fig, axs = plt.subplots(1, 3, figsize=(15, 5))

# We'll create x positions for the groups (0..4 for 5 ratios)
x = np.arange(len(ratios))  # array([0, 1, 2, 3, 4])
width = 0.35  # Width of each bar

# ===================== SUBPLOT 1: LOOP COUNTS ===================== #
axs[0].bar(x - width/2, loop_id00,  width, label='id00', color='tab:blue')
axs[0].bar(x + width/2, loop_id01,  width, label='id01', color='tab:orange')

axs[0].set_title('Loop Count vs. Critical-Section Ratio')
axs[0].set_xticks(x)
axs[0].set_xticklabels(ratios)
axs[0].set_xlabel('Critical-Section Ratio (id01:id00)')
axs[0].set_ylabel('Loop Count')
axs[0].legend()
axs[0].grid(True)

# (Optional) If values vary widely, you can use:
# axs[0].set_yscale('log')

# ==================== SUBPLOT 2: LOCK ACQUIRES ===================== #
axs[1].bar(x - width/2, acq_id00, width, label='id00', color='tab:blue')
axs[1].bar(x + width/2, acq_id01, width, label='id01', color='tab:orange')

axs[1].set_title('Lock Acquires vs. Critical-Section Ratio')
axs[1].set_xticks(x)
axs[1].set_xticklabels(ratios)
axs[1].set_xlabel('Critical-Section Ratio (id01:id00)')
axs[1].set_ylabel('Lock Acquires')
axs[1].legend()
axs[1].grid(True)

# (Optional) If values vary widely, you can use:
# axs[1].set_yscale('log')

# ==================== SUBPLOT 3: LOCK HOLD (us) ==================== #
axs[2].bar(x - width/2, hold_id00, width, label='id00', color='tab:blue')
axs[2].bar(x + width/2, hold_id01, width, label='id01', color='tab:orange')

axs[2].set_title('Lock Hold (us) vs. Critical-Section Ratio')
axs[2].set_xticks(x)
axs[2].set_xticklabels(ratios)
axs[2].set_xlabel('Critical-Section Ratio (id01:id00)')
axs[2].set_ylabel('Lock Hold Time (us)')
axs[2].legend()
axs[2].grid(True)

# (Optional) If values vary widely, you can use:
# axs[2].set_yscale('log')

# Adjust layout so labels/titles don't overlap
plt.tight_layout()

# Save to a file
plt.savefig('critical_section_bar_charts_new.png', dpi=300, bbox_inches='tight')

# If you also want to display it interactively:
# plt.show()
