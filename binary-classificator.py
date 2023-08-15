#
# Линейный бинарный классификатор
# Алгоритм оптимизации - градиентный спуск с вектором скользящего среднего
# Критерий обучения - порог изменения весов или метрики качества
# Метрика качества - доля правильных ответов
# Метод тестирования модели - QxT cross-validation
# Инициализирующий вектор - [-1/2n; +1/2n] случайный ИЛИ метод наименьших квадратов
#
# Ввод: предварительно подготовленная таблица с данными
# Вывод: точность классификации модели
#

import numpy as np
import pandas as pd

def L(M):
    if (M >= 0):
        return np.log(1+np.exp(-M))
    else:
        if (np.exp(M) == 0.0):
            return np.nan_to_num(np.inf)
        return np.nan_to_num(np.log((1+np.exp(M))/np.exp(M)))

def grad(w,x,y):
    g = np.array([], dtype=float)
    M = np.dot(w,x)*y
    if (M >= 0):
        sig = np.exp(-M)/(1+np.exp(-M))
    else:
        sig = 1/(1+np.exp(M))
    g = sig*y*(-x)
    return g

def train(w, x, y, r):
    t = 1.
    mw = 1
    mq = 1
    cw = 0
    cq = 0
    Q = 0
    for M in np.dot(x,w)*y:
        Q += L(M)
    Q = Q / len(x)
    # Stabilizing Vector
    G = np.zeros_like(x)
    for i in range(len(x)):
        G[i] = grad(w, x[i], y[i])
    S = G.sum(axis=0)
    #
    while (cw < 2) and (cq < 2):
        i = np.random.randint(0, len(x))
        lw = w.copy()
        lq = Q
        err = L(np.dot(x[i],w)*y[i])
        
        S = S - G[i]
        G[i] = grad(w, x[i], y[i])
        S = S + G[i]
        w = w * (1 - 1/t * r) - 1/t * S / (len(x))
        Q = 0.9*Q + 0.1*err
        
        mq = np.abs(Q - lq)
        mw = np.max(np.abs(w - lw))
        if (mw > 0.00001):
            cw = 0
        else:
            cw += 1
        if (mq > 0.0000001):
            cq += 0
        else:
            cq += 1
        t += 1
    #print("total_acc =", np.sum(np.sign(np.dot(x,w)) == y)/len(y), "\t M =", (np.dot(x,w)*y).mean(), "[t=", t, "]")
    return w

def CV(w,x,y,r):
    q = 4
    t = 5
    acc = 0
    acc_cl = 0
    for j in range(t):
        np.random.shuffle(x)
        for i in range(q):
            mask = np.full(len(y), True, dtype=bool)
            mask[i::q] = False
            cw = train(w.copy(),x[mask],y[mask],r)
            acc_n = (np.sign(np.dot(x, cw)) == y).sum() / len(y)
            acc_cl_n = (np.sign(np.dot(x[~mask], cw)) == y[~mask]).sum() / len(y[~mask])
            acc += acc_n
            acc_cl += acc_cl_n
            #print("acc =", acc_n, "\t clear =", acc_cl_n)
    print("r =", r, "\t Total accuracy =", acc/q/t, "\t Clear acc =", acc_cl/q/t)
        
    
data = pd.read_csv("../input/rdy/cabin_fin.csv", index_col='PassengerId')
x = data.to_numpy(dtype=float)
y = x[:,0].copy()
y = np.where(y == 0, -1, 1)
x = x[:,1:].copy()
#print("Strarting lsq...")
#w = np.linalg.lstsq(x,y, rcond=None)[0]
w = np.random.rand(x.shape[1])/9. - 1./18
print(w)
CV(w,x,y,0.28)