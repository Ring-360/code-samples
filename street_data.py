#
# Скрипт предназначен для построения карты улиц Москвы по известным данным системы городской транспортной инфраструктуры.
# В качестве входных данных используются:
# - таблица точек, содержащая координаты и данные о вершинах графа дорожной сети (каждая вершина - объект инфраструктуры, например, камера, светофор и т.д.)
# - таблица линий, содержащая описание дуг графа дорожной сети
# - таблица улиц, содержащая данные о названии улицы, её типе, типе дорожного покрытия и т.д.
# В качестве выходных данных генерируется json-файл в формате, предусмотренном Яндекс.Карты API, содержащий данные улиц для отображения на интерактивной карте.
#
# Скрипт использовался для подготовки к визуализации данных карты для облегчения поиска ошибок в исходных данных.
#

import pandas as pd
import hashlib
import json
linesPD = pd.read_csv("../input/compass/lines.csv", sep=';')
verticesPD = pd.read_csv("../input/compass/vertices.csv", sep=';')
streetPD = pd.read_csv("../input/compass/street.csv", sep=';')
vert = {}
for now in verticesPD.iloc:
    coord = now['geom'][7:-1].split(' ')
    vert[now['id']] = [float(coord[1]), float(coord[0])]
street = {}
for now in streetPD.iloc:
    street[now['id_street']] = {'name':now['LName'], 'col':'#'+hashlib.sha256(now['LName'].encode('utf-8')).hexdigest()[:6]}
res = {'type':'FeatureCollection', 'features':[]}
for key in vert:
    now = vert[key]
    res['features'].append({'type':'Feature', 'id':int(key), 'geometry':{'type':'Point', 'coordinates':now}, 'properties':{'balloonContent':'Point '+str(key)}, 'options':{'preset':'islands#circleIcon'}})
lineid = 100000
for now in linesPD.iloc:
    res['features'].append({'type':'Feature', 'id':lineid, 'geometry':{'type':'LineString', 'coordinates':[vert[now['N1']], vert[now['N2']]]}, 'properties':{'hintContent':street[now['id_street']]['name'],
                                                                                                                                      'balloonContent':'street_id = '+str(now['id_street'])+
                                                                                                                                       '<br>' + street[now['id_street']]['name']}, 'options':{'strokeColor':street[now['id_street']]['col'], 'strokeWidth':5}})
    lineid += 1
content = 'mapdata = \'' + json.dumps(res, ensure_ascii=False) + '\';'
with open('result.json', 'w') as f:
    f.write(content)