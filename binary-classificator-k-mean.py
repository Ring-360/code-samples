#
# Линейный 3-х классовый классификатор
# Алгоритм - метод простого голосования: ближайший сосед, метрика расстояния до объектов класса, дискриминантная функция (расстояние до разделяющей прямой).
# Решающее правило - последовательная дихотомия
# Метрика качества - доля правильных ответов
#
# Ввод: предварительно подготовленные обучающая и контрольная выборки, содержащие признаковые описания объектов 3-х классов.
# Вывод: точность классификации модели
#

import numpy as np
import pandas as pd

def result(data, x, i, j):
    res = np.argpartition(np.square(data[:, i-1] - x[i-1]) + np.square(data[:, j-1] - x[j-1]), 3)[:3]
    m2 = np.mean((data[(data[:,7] == -1)])[:, [i-1,j-1]], axis=0)
    m1 = np.mean((data[(data[:,7] == 1)])[:, [i-1,j-1]], axis=0)
    f = np.dot(m1-m2, x[[i-1, j-1]]) - 1/2 * np.dot(m1-m2, m1+m2)
    #print('f', m1-m2, m1+m2, 1/2 * np.dot(m1-m2, m1+m2))
    return np.sign(data[res[0],7] + np.sign(np.sum(data[res,7])) + np.sign(f))

data = pd.read_csv("../input/lab5-ok/5_1.csv").to_numpy(dtype=float)
data2 = pd.read_csv("../input/lab5-ok/5_2.csv").to_numpy(dtype=float)
perA = np.random.permutation(15)
perB = np.random.permutation(15)
perC = np.random.permutation(15)
test = data[np.append(np.append(perA[10:], perB[10:]+15), perC[10:]+30)]
data = data[np.append(np.append(perA[:10], perB[:10]+15),perC[:10]+30)]
data2 = data2[np.append(perA[:10], perB[:10]+15)]
acc = 0
for i in range(test.shape[0]):
    x = test[i, :7]
    res = -1
    if (result(data, x, 5, 6) > 0):
        res = 2
    elif (result(data2, x, 1, 4) > 0):
        res = 0
    else:
        res = 1
    if (i // 5 == res):
        acc += 1
print('accuracy = ', acc / 15.0)