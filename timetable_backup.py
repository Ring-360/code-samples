#
# Скрипт выгружает данные расписания учебных групп путём запросов к сайту университета с последующим парсингом HTML-страниц.
# Результатом работы является файл CSV-таблицы с данными о занятиях учебных групп в определённом формате. 
#

from htmldom import htmldom
import re

def get(str):
	a = re.search(r'(?u)\w(\w|\s\w|\.|\-)+', str)
	if a:
		return a.group(0)
	else:
		return ""
def getNum(str):
	a = re.search(r'(?u)\d+', str)
	if a:
		return a.group(0)
	else:
		return ""

groups = []
gr = htmldom.HtmlDom( "https://rut-miit.ru/timetable" ).createDom().find("div[id=ИУЦТ] a")._not("a[class=dropdown-toggle]")
for now in gr:
	groups.append({'group':get(now.text()), 'link':now.attr("href")})
group_id = 0
for cgroup in groups:
	if group_id > 200:
		exit()
	print("[ " + str(group_id+1) + " / " + str(len(groups)) + " ]   " + cgroup['group'])
	dom = htmldom.HtmlDom( "https://rut-miit.ru" + cgroup['link'] ).createDom()
	#dom = htmldom.HtmlDom( "https://rut-miit.ru/timetable/190034" ).createDom()
	rasp = []
	for weeks in range(2):
		w = str(weeks+1)
		week = dom.find("div[id=week-" + w + "] tr")
		days = []
		i = 0
		for row in week:
			if i == 0:
				for now in week.first().find("th"):
					days.append(get(now.text()))
				i += 1
				continue
			para = row.find("td[class=text-right]").text()[0]
			d = 1
			for now in row.find("td[class=timetable__grid-day]"):
				item = {}
				item['group'] = cgroup['group']
				item['lesson'] = now.find("div[class=timetable__grid-day-lesson]").text()
				if item['lesson'] == '':
					d += 1
					continue
				item['type'] = re.search(r'(?u)^(\w|\s\w|\.)+(\n|\,|\;)', item['lesson']).group(0)[:-1]
				item['lesson'] = re.search(r'(?u)\n(\w|\s\w|\.|\-|\"|\;)+', item['lesson']).group(0)[1:]
				blocks = now.find("div[class=mb-2]")
				block_count = 0
				for cblock in blocks:
					block_count += 1
				if block_count == 1:
					item['prepod'] = ""
					item['room'] = getNum(blocks.first().text())
				elif block_count == 0:
					item['prepod'] = ""
					item['room'] = ""
				else:
					item['prepod'] = get(blocks.first().text())
					item['room'] = getNum(blocks.eq(1).text())
				item['para'] = para
				item['day'] = days[d]
				item['week'] = w
				rasp.append(item)
				d += 1
			i += 1
	if group_id == 0:
		with open('test.csv', 'w+') as f:
			f.write(''.join( ('"' + val + '";') for val in rasp[0]) + '\n')
	with open('test.csv', 'a+') as f:
		f.seek(0,2)
		for item in rasp:
			f.write(''.join( ('"' + item[val].replace('"', '""') + '";') for val in item) + '\n')
	group_id += 1
exit()


groups = []
dom = htmldom.HtmlDom( "https://rut-miit.ru/timetable" ).createDom().find("div[id=ИУЦТ] a")._not("a[class=dropdown-toggle]")
for now in dom:
	groups.append({'group':get(now.text()), 'link':now.attr("href")})