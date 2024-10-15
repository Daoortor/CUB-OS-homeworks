from matplotlib import pyplot as plt
import subprocess

subprocess.call(['bash', './test.sh'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
with open('../../out.txt') as f:
    times = list(map(float, f.read().split()))

plt.plot(range(1, len(times) + 1), times, label="y = time(t) (average running time on t threads)")
plt.plot(range(1, len(times) + 1), [0.13 + (times[0] - 0.13) / t for t in range(1, len(times) + 1)], label="y = k/x + c")
plt.xlabel("# of threads")
plt.ylabel("time (s)")
plt.xlim(left=0)
plt.ylim(bottom=0)
plt.legend()
plt.show()
