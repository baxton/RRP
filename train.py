

import os
import sys
import numpy as np
import scipy as sp
import ctypes
from array import array
import datetime as dt

from sklearn.ensemble import RandomForestClassifier


#ANN_DLL = ctypes.cdll.LoadLibrary(r"/home/maxim/kaggle/RRP/scripts/ann/libann.so")
ANN_DLL = ctypes.cdll.LoadLibrary(r"C:\Temp\test_python\RRP\scripts\ann_t\ann2.dll")


#path_data = "/home/maxim/kaggle/RRP/data/"
path_data = "C:\\Temp\\test_python\\RRP\\data\\"

fname_train = "train_data.csv"
fname_test  = "test_data.csv"

REV_MEAN = 4453532.6131386859

ARR_LEN = 44
VEC_LEN = 41


outlier_detector_trained = False
rf_outliers = None
FX = None


NULL = .0001

Rmin = -10.5
Rmax = 10.5

YRmin = .3
YRmax = .7


x_beg = 0
x_end = -1



def minmax(min1, max1, min2, max2, X):

    if min1 == None or max1 == None:
        min1 = X.min(axis=0)
        max1 = X.max(axis=0)

    k = (max2 - min2) / (max1 - min1)
    X -= min1
    X *= k
    X += min2

    return min1, max1, X





def get_k_of_n(k, n):
    numbers = np.array([0] * k, dtype=int)

    for i in range(k):
        numbers[i] = i

    for i in range(k, n):
        r = np.random.randint(0, i)
        if r < k:
            numbers[r] = i

    return numbers


def choice(arr, k):
    n = len(arr)
    indices = get_k_of_n(k, n)


    if isinstance(arr, np.ndarray):
        return arr[indices]

    t = type(arr[0])
    return np.array([arr[i] for i in indices], dtype=t)



def load(fname):
    data = np.loadtxt(fname, delimiter=',')
    return data.astype(np.float64)



def augment_one(X, Y, idx, num=10):
    x = X[idx,:].copy()
    y = Y[idx,:].copy()

    #vals = X.max(axis=0)

    for i in range(num):
        for j in range(5):
            rc = sp.random.randint(0, x.shape[0], 1)
            x[rc] = NULL
        X = np.append(X, x.reshape((1,x.shape[0])), axis=0)
        Y = np.append(Y, y.reshape((1,y.shape[0])), axis=0)
    return X, Y




def train_rf_for_outliers(train_data, Y):

    mean = Y.mean()
    std = Y.std()

    rf = RandomForestClassifier(n_estimators=1000, bootstrap=False)

    res = np.zeros((train_data.shape[0],1))
    for i in range(train_data.shape[0]):
        v = Y[i]
        if np.sqrt((v - mean) ** 2) > (std * 1.):
            res[i] = 1.

    rf.fit(train_data, res)

    return rf, res



def train_bias_remover(ann, X, Y, train_set):
    prediction = np.array([0],dtype=np.float64)
    Ytmp = Y.copy()
    for i in train_set:
        x = X[i,:]
        ANN_DLL.ann_predict(ctypes.c_void_p(ann), x.ctypes.data, prediction.ctypes.data, ctypes.c_int(1))
        d = prediction[0] - Y[i,0]
        Ytmp[i] = d

    Ytmp = Ytmp[train_set].astype(np.float64)
    Xtmp = X[train_set].astype(np.float64)
    MBS  = len(train_set)
    alpha = ctypes.c_double(.8)

    Ytmp = np.append(Ytmp, Ytmp, axis=0)
    Xtmp = np.append(Xtmp, Xtmp, axis=0)

    sizes = np.array([X.shape[1]] + [35]*1 + [1], dtype=np.int32)
    ann_bias = ANN_DLL.ann_create(sizes.ctypes.data, ctypes.c_int(sizes.shape[0]), ctypes.c_int(1))

    for i in range(1):
        ANN_DLL.ann_fit(ctypes.c_void_p(ann_bias), Xtmp.ctypes.data, Ytmp.ctypes.data, ctypes.c_int(MBS), ctypes.addressof(alpha), ctypes.c_double(.125), ctypes.c_int(500))

    return ann_bias

def prep_data(X, p=1.):
    tmp = X.copy()

    #X = np.append(X, np.tan(tmp), axis=1)


    #X = np.append(X, tmp**2, axis=1)
    #X = np.append(X, tmp**3, axis=1)
    #X = np.append(X, tmp**4, axis=1)
    #X = np.append(X, tmp**5, axis=1)
    tmp[ tmp == 0.] = .000001

    #X = np.append(X, 1. / tmp, axis=1)

##    X = np.append(X, np.sin(tmp), axis=1)

##    S = 32
##    N = 12
##    m = []
##    for r in range(X.shape[0]):
##        row = []
##        for c1 in range(S, S+N):
##            for c2 in range(c1+1, S+N):
##                row.append(tmp[r,c1] / tmp[r,c2])
##
##        m.append(row)
##    X = np.append(X, np.array(m, dtype=np.float64), axis=1)

    P = 13  # 10, 13, 14, 15, 16, 18, 19, 34, 35, 36
    m = []
    for r in range(tmp.shape[0]):
        row = []
        n = tmp[r, P]
        for c2 in range(7, tmp.shape[1]):
            if c2 != P:
                row.append(n / tmp[r,c2])

        m.append(row)
    X = np.append(X, np.array(m, dtype=np.float64), axis=1)

##    P = 16  # 10, 13, 14, 15, 16, 18, 19, 34, 35, 36
##    m = []
##    for r in range(tmp.shape[0]):
##        row = []
##        n = tmp[r, P]
##        for c2 in range(7, tmp.shape[1]):
##            if c2 != P:
##                row.append(n / tmp[r,c2]**2)
##
##        m.append(row)
##    X = np.append(X, np.array(m, dtype=np.float64), axis=1)



##    S = 25
##    N = 35
##    m = []
##    for r in range(X.shape[0]):
##        row = []
##        for c1 in range(S, S+N):
##            for c2 in range(c1, S+N):
##                row.append(tmp[r,c1] * tmp[r,c2])
##
##        m.append(row)
##    X = np.append(X, np.array(m, dtype=np.float64), axis=1)


#    X = np.append(X, np.log(1. + np.sin(tmp)), axis=1)
    return X


def process3(train_data, out_id):


    data = train_data

    Y = data[:,-1].copy()

    # preproc
    Y_LEN = 1

    Y = Y.reshape((Y.shape[0],Y_LEN))

    N = Y.shape[0]
    train_set = [n for n in range(N) if n != out_id]
    sp.random.shuffle(train_set)
    test_set = [out_id]

##    test_set =  [23, 2, 27, 99, 80, 97, 114, 131, 124]
##    train_set =  [0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 24, 25, 26, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 98, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 115, 116, 117, 118, 119, 120, 121, 122, 123, 125, 126, 127, 128, 129, 130, 132, 133, 134]

    X = data[:,x_beg:x_end].copy()
##    X = np.append(X, FX, axis=1)


    X = prep_data(X)

#    for i in choice(range(Y.shape[0]), Y.shape[0] / 3 * 2):
#    for i in range(Y.shape[0]):
#        X, Y = augment_one(X, Y, i, num=1)

    Ymin, Ymax, Y = minmax(None, None, YRmin, YRmax, Y)
    Xmin, Xmax, X = minmax(None, None, Rmin, Rmax, X)


    global outlier_detector_trained, rf_outliers, FX
    tmp = np.zeros((X.shape[0],1))
    X = np.append(X, tmp, axis=1)

    if not outlier_detector_trained:
        rf_outliers, FX = train_rf_for_outliers(X[train_set,:], Y[train_set])
        outlier_detector_trained = True

        p = rf_outliers.predict(X[out_id,:])
        X[out_id, -1] = p

    for i, idx in enumerate(train_set):
        X[idx] = FX[i]


    #
    #
    #



#    sizes = np.array([X.shape[1]] + [30]*1 + [1], dtype=np.int32)
    sizes = np.array([X.shape[1]] + [30]*1 + [1], dtype=np.int32)
    ann = ANN_DLL.ann_create(sizes.ctypes.data, ctypes.c_int(sizes.shape[0]), ctypes.c_int(1))
    ##lr = LinearRegression()

    if 0 == len(test_set):
        test_set = train_set

##    inner_iter = 3
##    l = .125
##    alpha = ctypes.c_double(8.)
##    MBS = int(len(train_set) * .85)

    inner_iter = 1
    l = .12
    alpha = ctypes.c_double(.8)
    MBS = int(len(train_set) * .85)

    prediction = np.array([0]*1, dtype=np.float64)


    cost = 999999999.
    best_cost = 999999999.
    prev_cost = 999999999.
    cost_cnt = 0

    weights_saved = False

    indices = train_set
    np.random.shuffle(indices)

    for i in range(2000):
        #indices = train_set[:MBS]
        indices = choice(train_set, MBS)
        ##np.append(indices, choice(train_set, MBS / 3))
#        indices = np.append(indices, choice(train_set, MBS / 3 * 2))

        #MBS = len(train_set) if 0 == (i % 2) else len(train_set) / 3 * 2

        Ytmp = Y[indices,:].astype(np.float64)
        Xtmp = X[indices,:].astype(np.float64)

        ANN_DLL.ann_fit(ctypes.c_void_p(ann), Xtmp.ctypes.data, Ytmp.ctypes.data, ctypes.c_int(MBS), ctypes.addressof(alpha), ctypes.c_double(l), ctypes.c_int(inner_iter))
#        alpha.value = .008
##        if i > 400000:
##            alpha.value = .00002
##        elif i > 250000:
##            alpha.value = .00004
##        elif i > 200000:
##            alpha.value = .00006
##        elif i > 140000:
##            alpha.value = .00008
##        elif i > 100000:
##            alpha.value = .0001
##        elif i > 80000:
##            alpha.value = .0002
##        elif i > 60000:
##            alpha.value = .0004
##        elif i > 50000:
##            alpha.value = .0006
##        elif i > 40000:
##            alpha.value = .0008
##        elif i > 30000:
##            alpha.value = .0015
##        elif i > 20000:
##            alpha.value = .001
##        elif i > 10000:
##            alpha.value = .002
##        elif i > 1000:
##            alpha.value = .004
####        elif i > 40:
####            alpha.value = .01
####        elif i > 30:
####            alpha.value = .02
####        elif i > 20:
####            alpha.value = .04
##        elif i > 10:
##            alpha.value = .009


        ## COST
        if i > 0 and 0 == (i % 200):
            print "MBS:", MBS, "ITER:", i

            cost = 0.
            for i in test_set:
                x = X[i,:].astype(np.float64)
                ANN_DLL.ann_predict(ctypes.c_void_p(ann), x.ctypes.data, prediction.ctypes.data, ctypes.c_int(1))
                m1, m2, v = minmax(YRmin, YRmax, Ymin, Ymax, prediction[0])
                m1, m2, y = minmax(YRmin, YRmax, Ymin, Ymax, Y[i,0])
                #v = prediction[0]
                #y = Y[i,0]
                cost += (v - y) * (v - y)
                ##print prediction, v, "\t", y, "(", v - y , ")", i
            cost /= len(test_set)
            cost = np.sqrt(cost)
            print "COST:", cost, "Rmin/max", Rmin, Rmax, "out_id", out_id

            if prev_cost < cost:
                #alpha.value /= 2.
                cost_cnt += 1
                if cost_cnt > 10:
                    #break
                    #ANN_DLL.ann_shift(ctypes.c_void_p(ann))
                    pass
            else:
                if cost < best_cost:
                    best_cost = cost
                    ANN_DLL.ann_save(ctypes.c_void_p(ann))
                    weights_saved = True
                cost_cnt = 0
            prev_cost = cost

##        if i > 0 and 0 == (i & 1000) or cost < 1100000 or cost > 1900000:
##            ANN_DLL.ann_shift(ctypes.c_void_p(ann))
##            l += .1
##            alpha.value = 0.008
##            #inner_iter += 1

#            if cost < 900000. or cost > 1100000.:
#                break



            #break


        if alpha.value == 0:
            break;

##        if 4 >= sp.random.randint(0, 10, 1):
##            alpha.value *= sp.random.randint(2, 5, 1)


        if True == os.path.exists("C:\\Temp\\test_python\\RRP\\scripts\\ann_t\\STOP.txt"):
            break

    if weights_saved:
        ANN_DLL.ann_restore(ctypes.c_void_p(ann))

    #ann_bias = train_bias_remover(ann, X, Y, test_set)
    ann_bias = None

    pbias = np.array([0], dtype=np.float64)

    # COST last one
    cost = 0.
    for i in test_set:
#    for i in train_set:
        x = X[i,:].astype(np.float64)
        ANN_DLL.ann_predict(ctypes.c_void_p(ann), x.ctypes.data, prediction.ctypes.data, ctypes.c_int(1))

        ##ANN_DLL.ann_predict(ctypes.c_void_p(ann_bias), x.ctypes.data, pbias.ctypes.data, ctypes.c_int(1))

        m1, m2, v = minmax(YRmin, YRmax, Ymin, Ymax, prediction[0] - pbias[0])
        m1, m2, y = minmax(YRmin, YRmax, Ymin, Ymax, Y[i,0])
        #v = prediction[0]
        #y = Y[i,0]
        cost += (v - y) * (v - y)
        ##print prediction, v, "\t", y, "(", v - y , ")", i
    cost /= len(test_set)
    cost = np.sqrt(cost)
    print "COST:", cost, "Rmin/max", Rmin, Rmax, "(best)", best_cost


    return ann, ann_bias, rf_outliers, cost, Xmin, Xmax, Ymin, Ymax
    ##


def regression(ann, ann_bias, rf_outliers, test_data, Xmin, Xmax, Ymin, Ymax, fnum, cost, pref):
    # regression
    data = test_data

    X = data[:,x_beg:x_end].copy()

    X = prep_data(X)

    print "REG: Xmin/max", Xmin, Xmax
    m1, m2, X = minmax(Xmin, Xmax, Rmin, Rmax, X)

##    FX = rf_outliers.predict(X)
##    X = np.append(X, FX.reshape(FX.shape[0],1), axis=1)


    vals = np.zeros((X.shape[0],))

    prediction = np.array([0]*1, dtype=np.float64)
    pbias = np.array([0], dtype=np.float64)

    with open(path_data + "../submission_%s_%f_%d.txt" % (pref, cost, fnum), "w+") as fout:
        fout.write("Id,Prediction%s" % os.linesep)
        for row in range(data.shape[0]):
            x = X[row,:].astype(np.float64)
            ANN_DLL.ann_predict(ctypes.c_void_p(ann), x.ctypes.data, prediction.ctypes.data, ctypes.c_int(1))
            if None != ann_bias:
                ANN_DLL.ann_predict(ctypes.c_void_p(ann_bias), x.ctypes.data, pbias.ctypes.data, ctypes.c_int(1))

            #v = prediction[0] * (Ymax - Ymin) # + Ymean
            m1, m2, v = minmax(YRmin, YRmax, Ymin, Ymax, prediction[0] - pbias[0])
            #v = prediction[0]

            if 0 == (row % 5000):
                print "ID:", data[row,-1], "val:", v
            fout.write("%d,%2.16f%s" % (data[row,-1], v, os.linesep))

            vals[row] = v

        print "ID:", data[row,-1], "val:", v
        print "STD:", vals.std(), "MEAN:", vals.mean(), "NEGS:", len(vals[vals < 0.])






def main():
    sp.random.seed()

    pref = sys.argv[1]
    #pref = "xxx"

    train = load(path_data + fname_train)
    test = load(path_data + fname_test)

    ROWS_NUM = train.shape[0]

    # remove outliers
    ii = train[:,-1] < 15000000.
    train = train[ii,:]


    global Rmin, Rmax
    #Rmin = -1.5
    #Rmax = 1.5

    Xmin = 0.
    Xmax = 0.

    ann = None

    fnum = 0
    cost = 0.

    flog = open(pref + "_test.txt", "w+")

    N = 1
    cnt = 0.
    for i in range(0, N):
        for r in range(train.shape[0]):
            ann, ann_bias, rf_outliers, tmp_cost, Xmin, Xmax, Ymin, Ymax = process3(train, r)
            flog.write("%d %f (%f)\n" % (r, tmp_cost, train[r,-1]))
            flog.flush()

            if True:  # tmp_cost < 1000000:
                cost += tmp_cost
                ##regression(ann, ann_bias, rf_outliers, test, Xmin, Xmax, Ymin, Ymax, fnum, tmp_cost, pref)
                fnum += 1

        ANN_DLL.ann_free(ctypes.c_void_p(ann))
        #ANN_DLL.ann_free(ctypes.c_void_p(ann_bias))

        if True == os.path.exists("C:\\Temp\\test_python\\RRP\\scripts\\ann_t\\STOP.txt"):
            break
    flog.write("COST: %f\n" % cost)
    flog.close()
    cost /= fnum

    print "AVR COST:", cost



   # train_classifier(train)



if __name__ == '__main__':
    main()




#
# Ymean = 4453532.49635; Ymin = -3.30366e+06; Ymax = 1.52434e+07
#

