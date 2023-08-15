#
# Скрипт обработки данных с датчиков поезда по заданию Рутакона.
# Вход: данные датчиков в бинарных файлах и файлах .hdf5
# Выход: приведённые к единому формату, точке синхронизации и интерполированные данные в единой таблице формата parquet.
#

import numpy as np
import pandas as pd
import h5py as hdf
import vaex
import sys
import os, fnmatch
def find(pattern, path):
    result = []
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                result.append(os.path.join(root, name))
    return result

print('RUTACON v0.0.1 build 978616864684106517684615607687864168561116898461 is loading...')

f1 = np.array([], dtype=np.float64).reshape((0,2))
f2 = np.array([], dtype=np.float64).reshape((0,2))
f3 = np.array([], dtype=np.float64).reshape((0,2))
gps = np.array([], dtype=np.float64).reshape((0,9))
directory = '/kaggle/input/rutacon/data_09_26/'
isInBinaryMode = False
temp = np.zeros((20000000, 2), dtype=np.float64)
tempptr1 = 0
tempptr2 = 0
tempptr3 = 0

files = find('*.hdf', directory)
if (len(files) > 0):
    print('Found .hdf5 file; RUTACON is now working in HDF mode.')
    with hdf.File('/kaggle/input/rutacon/data_2022_10_16.hdf', 'r') as f:
        print('Loading file... This may take a while')
        f1 = f['FrontCart.reducer.1.phase_1']['data'][()]
        f1[:, 0] = f1[:,0] * 1000
        f2 = f['FrontCart.reducer.1.phase_2']['data'][()]
        f2[:, 0] = f2[:,0] * 1000
        f3 = f['FrontCart.reducer.1.phase_3']['data'][()]
        f3[:, 0] = f3[:,0] * 1000
        gps = f['GPS_1']['data'][()]
else:
    isInBinaryMode = True
    files = find('*401.bin', directory)
    if (len(files) > 0):
        print('Found .bin file; RUTACON is now working in BINARY mode.')
        cnt = 0
        print('Loading PHASE_1 files... This may take a while')
        for now in files:
            f = open(now, "rb")
            f.seek(0x147, os.SEEK_SET)
            t1 = np.fromfile(f, dtype = np.dtype([
                ('time', np.int64),
                ('phase_1', np.float32)
            ]))
            for item in t1:
                temp[tempptr1] = [item[0] * 1000, item[1]]
                tempptr1 += 1
            cnt += 1
            if (cnt % 10 == 0):
                print('PHASE_1', cnt, 'out of', len(files), 'processed')
            f.close()
        f1 = temp[:tempptr1]
    else:
        sys.exit('No files to process!')
    files = find('*402.bin', directory)
    if (len(files) > 0):
        cnt = 0
        print('Loading PHASE_2 files... This may take a while')
        for now in files:
            f = open(now, "rb")
            f.seek(0x147, os.SEEK_SET)
            t1 = np.fromfile(f, dtype = np.dtype([
                ('time', np.int64),
                ('phase_1', np.float32)
            ]))
            for item in t1:
                temp[tempptr2] = np.array([item[0] * 1000, item[1]])
                tempptr2 += 1
            cnt += 1
            if (cnt % 10 == 0):
                print('PHASE_2', cnt, 'out of', len(files), 'processed')
            f.close()
        f2 = temp[:tempptr2]
    else:
        sys.exit('Not enough files! Found PHASE_1, but no PHASE_2')
    files = find('*403.bin', directory)
    if (len(files) > 0):
        print('Loading PHASE_3 files... This may take a while')
        cnt = 0
        for now in files:
            f = open(now, "rb")
            f.seek(0x147, os.SEEK_SET)
            t1 = np.fromfile(f, dtype = np.dtype([
                ('time', np.int64),
                ('phase_1', np.float32)
            ]))
            for item in t1:
                temp[tempptr3] = np.array([item[0] * 1000, item[1]])
                tempptr3 += 1
            cnt += 1
            if (cnt % 10 == 0):
                print('PHASE_3', cnt, 'out of', len(files), 'processed')
            f.close()
        f3 = temp[:tempptr3]
    else:
        sys.exit('Not enough files! Found PHASE_1-2, but no PHASE_3')
    files = find('Mikro*.bin', directory)
    if (len(files) > 0):
        cnt = 0
        print('Loading GPS_1 files... This may take a while')
        for now in files:
            f = open(now, "rb")
            f.seek(0x43, os.SEEK_SET)
            t1 = np.fromfile(f, dtype = np.dtype([
                ('TIME', np.int64),
                ('GPS_TIME', np.int64),
                ('LAT', np.float64),
                ('LON', np.float64),
                ('ALT', np.float64),
                ('SPEED', np.float64),
                ('BEARING', np.float64),
                ('HDOC', np.float64),
                ('UNUSED', np.float64)
            ]))
            for item in t1:
                gps = np.append(gps, np.array([np.float64(item[0]),
                                              np.float64(item[1]),
                                              np.float64(item[2]),
                                              np.float64(item[3]),
                                              np.float64(item[4]),
                                              np.float64(item[5]),
                                              np.float64(item[6]),
                                              np.float64(item[7]),
                                              np.float64(0)]).reshape((1,9)), axis=0)
            cnt += 1
            if (cnt % 10 == 0):
                print('GPS_1', cnt, 'out of', len(files), 'processed')
            f.close()
    else:
        sys.exit('Not enough files! Found PHASE_1-3, but no GPS_1')
    f1 = f1[f1[:,0].argsort()]
    f2 = f2[f2[:,0].argsort()]
    f3 = f3[f3[:,0].argsort()]
    gps = gps[gps[:,0].argsort()]

print('Data loading complete; Going forward to process...')
st = np.max(np.array([f1[0,0], f2[0,0], f3[0,0], gps[0,0]*1000000]))
fin = np.min(np.array([f1[-1,0], f2[-1,0], f3[-1,0], gps[-1,0]*1000000]))
mask = np.full(9, False)
mask[0] = True
mask[5] = True

f1 = pd.DataFrame(data=f1, columns=['time', 'phase_1']).astype({'time':np.float64, 'phase_1':np.float64})
f1['time'] = pd.to_datetime(f1['time'], unit='ns', origin='unix')
f1 = f1.set_index('time')
print('PHASE_1 unit data processed')

f2 = pd.DataFrame(data=f2, columns=['time', 'phase_2']).astype({'time':np.float64, 'phase_2':np.float64})
f2['time'] = pd.to_datetime(f2['time'], unit='ns', origin='unix')
f2 = f2.set_index('time')
print('PHASE_2 unit data processed')

f3 = pd.DataFrame(data=f3, columns=['time', 'phase_3']).astype({'time':np.float64, 'phase_3':np.float64})
f3['time'] = pd.to_datetime(f3['time'], unit='ns', origin='unix')
f3 = f3.set_index('time')
print('PHASE_3 unit data processed')

gps = pd.DataFrame(data=gps[:, mask], columns=['time', 'speed']).astype({'time':np.float64, 'speed':int})
gps['time'] = pd.to_datetime(gps['time'], unit='ms')
gps = gps.set_index('time')
print('GPS_1 unit data processed')

print('Setting up the resulting table...')
dt = pd.date_range(start=pd.to_datetime(st, unit='ns', origin='unix'), end=pd.to_datetime(fin, unit='ns', origin='unix'), freq='1ms')
res = pd.DataFrame(index=dt)
res.index = res.index.rename('time')

print('Resulting table was set up successfully')
res = pd.merge_asof(res, f1, on='time', tolerance=pd.Timedelta('1ms'))
res = res.set_index('time')
print('PHASE_1 unit data merged')
res = pd.merge_asof(res, f2, on='time', tolerance=pd.Timedelta('1ms'))
res = res.set_index('time')
print('PHASE_2 unit data merged')
res = pd.merge_asof(res, f3, on='time', tolerance=pd.Timedelta('1ms'))
res = res.set_index('time')
print('PHASE_3 unit data merged')
res = pd.merge_asof(res, gps.sort_values('time'), on='time', tolerance=pd.Timedelta('1ms'))
res = res.set_index('time')
print('GPS_1 unit data merged')
print('Starting data interpolation...')
if (pd.isna(res.iloc[0,0])):
    res.iloc[0,0] = 0
if (pd.isna(res.iloc[0,1])):
    res.iloc[0,1] = 0
if (pd.isna(res.iloc[0,2])):
    res.iloc[0,2] = 0
if (pd.isna(res.iloc[0,3])):
    res.iloc[0,3] = 0
res = res.interpolate(method='time')
print('Data interpolation complete')
t = np.arange(np.float64(0), np.float64(len(dt)*1000000), np.float64(1000000))
res['t'] = t
print('Creating the result file')
res.to_parquet('/kaggle/working/result.parquet.gzip', compression='gzip', index=False)
print('Complete! The result file .parquet in /...')