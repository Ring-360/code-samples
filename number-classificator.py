#
# Линейный бинарный классификатор с синтетической генерацией выборки и обеспечением помехоустойчивости.
#
# Задача - определить цифру 0 по её графическому изображению на 7x5 индикаторе.
# Алгоритм оптимизации - градиентный спуск
# Критерий обучения - количество изменений вектора w
# Метрика качества - доля правильных ответов
# Метод тестирования модели - QxT cross-validation
# Инициализирующий вектор - нули
# Генерация выборки - статистически равномерная выборка со случайным внесением инвертирующих помех до заданного уровня
#
# Ввод: идеальные признаковые описания графического представления цифр на 7x5 индикаторе (бинарные признаки)
# Вывод: средняя точность модели, средние число итераций обучения, худший случай, лучший случай, график зависимости точности от доли помех в выборке.
#

import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import make_interp_spline, BSpline

def train(data, y, w):
    no_change = 0
    for i in range(10000):
        j = np.random.randint(data.shape[0])
        if (y[j] * np.dot(data[j], w) <= 0):
            w = w + y[j] * data[j]
            no_change = 0
        else:
            no_change += 1
        if (no_change > 50):
            break
    acc = np.sum(np.sign(np.dot(data, w) * y) == 1) / data.shape[0]
    #print(acc, ';', i)
    return w, i

def setupData(a, err):
    data = np.array([]).reshape((0,a.shape[1]))
    y = np.array([])
    for i in range(10000):
        num = i % 10
        t = a[num].copy()
        #per = np.random.permutation(np.arange(a.shape[1]))
        #for j in range(err):
        #    if (t[per[j]] == 0):
        #        t[per[j]] = 1
        #    else:
        #        t[per[j]] = 0
        for j in range(a.shape[1]):
            if (np.random.random() < err/a.shape[1]):
                if (t[j] == 0):
                    t[j] = 1
                else:
                    t[j] = 0
        data = np.append(data, t.reshape((1,a.shape[1])), axis=0)
        y = np.append(y, 1 if num == 5 else -1)
    return data, y

def CV(x, y, w):
    t = 40
    q = 10
    acc_t = 0
    iter_t = 0
    best = [0, 0]
    worst = [1, 0]
    acc_ov = 0
    for j in range(t):
        per = np.random.permutation(np.arange(len(x)))
        x = x[per]
        y = y[per]
        for i in range(q):
            mask = np.full(len(x), True, dtype=bool)
            mask[i::q] = False
            w_trained, it = train(x[mask], y[mask], w.copy())
            acc = np.sum(np.sign(y[~mask] * np.dot(x[~mask], w_trained)) == 1)/len(x[~mask])
            best = [max(best[0], acc), it if best[0] < acc else best[1]]
            worst = [min(worst[0], acc), it if acc < worst[0] else worst[1]]
            acc_t += acc
            acc_ov += np.sum(np.sign(y[mask] * np.dot(x[mask], w_trained)) == 1)/len(x[mask])
            iter_t += it
    print('acc_avg = ', acc_t/(q*t), ' iter_avg = ', iter_t/(q*t))
    print('worst_case = ', worst, 'best_case', best)
    return np.array([acc_t/(q*t),worst[0], best[0]])

def getAllResults(a, w, lst=False):
    res = np.zeros((a.shape[1]+1, 3))
    for i in range(a.shape[1]+1):
        data, y = setupData(a, i)
        if (lst):
            w = np.linalg.lstsq(data, y, rcond=None)[0]
        print(i, 'errs (', i/(a.shape[1]+1)*100, '%):')
        res[i] = 1 - CV(data, y, w.copy())
        print(res[i])
    return res

'''
a = np.array([[1,1,1,1,1,1,0,0,0], #0
              [0,1,1,0,0,0,0,1,0], #1
              [1,1,0,1,0,0,0,0,1], #2
              [1,0,0,0,0,0,1,1,1], #3
              [0,1,1,0,0,1,1,0,0], #4
              [1,0,1,1,0,1,1,0,0], #5
              [0,0,1,1,1,0,1,1,0], #6
              [1,0,0,0,1,0,0,1,0], #7
              [1,1,1,1,1,1,1,0,0], #8
              [1,1,0,0,0,1,1,0,1]]) #9
'''
a = np.array([[1,1,1,1,1,
               1,0,0,0,1,
               1,0,0,0,1,
               1,0,0,0,1,
               1,0,0,0,1,
               1,0,0,0,1,
               1,1,1,1,1],
              
              [0,0,0,1,1,
               0,0,1,0,1,
               0,1,0,0,1,
               0,0,0,0,1,
               0,0,0,0,1,
               0,0,0,0,1,
               0,0,0,0,1],
              
              [1,1,1,1,1,
               0,0,0,0,1,
               0,0,0,0,1,
               0,0,1,1,1,
               0,1,0,0,0,
               1,0,0,0,0,
               1,1,1,1,1],
              
              [1,1,1,1,0,
               0,0,0,0,1,
               0,0,0,0,1,
               1,1,1,1,0,
               0,0,0,0,1,
               0,0,0,0,1,
               1,1,1,1,0],
             
              [1,0,0,0,1,
               1,0,0,0,1,
               1,0,0,0,1,
               1,1,1,1,1,
               0,0,0,0,1,
               0,0,0,0,1,
               0,0,0,0,1],
             
             [1,1,1,1,1,
              1,0,0,0,0,
              1,0,0,0,0,
              1,1,1,1,1,
              0,0,0,0,1,
              0,0,0,0,1,
              1,1,1,1,1],
             
             [1,1,1,1,1,
              1,0,0,0,0,
              1,0,0,0,0,
              1,1,1,1,1,
              1,0,0,0,1,
              1,0,0,0,1,
              1,1,1,1,1],
             
             [1,1,1,1,1,
              0,0,0,0,1,
              0,0,0,0,1,
              0,0,0,0,1,
              0,0,0,0,1,
              0,0,0,0,1,
              0,0,0,0,1],
             
             [1,1,1,1,1,
              1,0,0,0,1,
              1,0,0,0,1,
              1,1,1,1,1,
              1,0,0,0,1,
              1,0,0,0,1,
              1,1,1,1,1],
             
             [1,1,1,1,1,
              1,0,0,0,1,
              1,0,0,0,1,
              1,1,1,1,1,
              0,0,0,0,1,
              0,0,0,0,1,
              1,1,1,1,1]])

fig, ax = plt.subplots()
ax.set_xlabel('Ошибок в выборке')
ax.set_ylabel('Потери')
ax.set_title('Ошибки - 5x7 цифра')
xnew = np.linspace(0, a.shape[1]+1, 300)
ax.plot(xnew, make_interp_spline(np.arange(a.shape[1]+1), getAllResults(a, np.zeros(a.shape[1])))(xnew), label='null vec')
res = getAllResults(a, np.zeros(a.shape[1]))
ax.plot(np.arange(a.shape[1]+1), res[:,0], label='в среднем', linewidth=2.0)
ax.plot(np.arange(a.shape[1]+1), res[:,1], label='в худшем случае', linewidth=2.0)
ax.plot(np.arange(a.shape[1]+1), res[:,2], label='в лучшем случае', linewidth=2.0)
ax.plot(np.arange(a.shape[1]+1), getAllResults(a, np.random.randint(10, size=a.shape[1])), label='случ. числа', linewidth=2.0)
ax.plot(np.arange(a.shape[1]+1), getAllResults(a, 1, True), label='МНК решение', linewidth=2.0)
ax.set_xlim(xmin=0, xmax=a.shape[1])
ax.set_ylim(ymin=0)
ax.legend()
plt.savefig('test.png')



a = np.append(a, np.ones(10).reshape((10,1)), axis=1)

data, y = setupData(a, 4)
w = np.zeros(a.shape[1])
CV(data, y, w)