#
# Использовалось для визуализации данных при проверке какой-то проблемы.
#

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

res = pd.read_parquet('./result.parquet (3).gzip')
res['phase_1'].plot(legend=True)
res['phase_2'].plot(legend=True)
res['phase_3'].plot(legend=True)
res['speed'].plot(legend=True)
p1 = pd.read_parquet('./p1.parquet')
p2 = pd.read_parquet('./p2.parquet')
p3 = pd.read_parquet('./p3.parquet')
print(p1)
print(p2)
rint(p3)
p1['value'][100000:150000].plot()
p2['value'][100000:150000].plot()
p3['value'][100000:150000].plot()
plt.show()