#
# Данный скрипт моделирует игру в блэкджек по базовой стратегии при игре с одной колодой.
# Предназначался для сверки теоретических и практически полученных статистических результатов игры.
#
# Ввод: карты игрока и открытая карта диллера. Например, '7 A' и '5'.
# Вывод: кол-во сыгранных рук, побед, ничьих, шансы и очки.
#

import numpy as np

def getTotalValue(stack):
	s = 0
	tuz = 0
	for now in stack:
		if (now == 1):
			tuz += 1
		elif (now >= 11):
			s += 10
		else:
			s += now
	if (tuz > 0):
		if (s + 11 + (tuz-1) <= 21):
			s += 11 + (tuz-1)
		else:
			s += tuz
	return s

stat = [0]*14
gstat = 0
deck = np.repeat(np.arange(1,14, dtype=int), 4)
np.random.shuffle(deck)
def getCard():
	global stat, gstat, deck
	#gen = np.random.randint(1, 14)
	gen = deck[0]
	deck = np.delete(deck, 0)
	if (deck.size <= 13):
		deck = np.repeat(np.arange(1,14, dtype=int), 4)
		np.random.shuffle(deck)

	stat[gen] += 1
	gstat += 1
	return gen

cards = {1: 'A', 11: 'J', 12: 'Q', 13: 'K'}
bcards = {'A': 1, 'J': 11, 'Q': 12, 'K': 13}

def hand2str(hand):
	res = '['
	for now in hand:
		if now in cards:
			res += cards[now]
		else:
			res += str(now)
		res += ', '
	return res[:-2] + ']'


def testSplit(hand, b):
	if (len(hand) == 2) and (hand[0] == hand[1]):
			if ((hand[0] == 1) or (hand[0] == 8)) and (b[0] != 1):
				return True
			if (hand[0] == 4) and ((b[0] == 5) or (b[0] == 6)):
				return True
			if (hand[0] == 9) and ((b[0] >= 2 and b[0] <=6) or (b[0] >= 8 and b[0] <= 9)):
				return True
			if (hand[0] == 6) and (b[0] >= 2 and b[0] <= 6):
				return True
			if (hand[0] == 2 or hand[0] == 3 or hand[0] == 7) and (b[0] >= 2 and b[0] <= 7):
				return True
	return False

def play(a, b):
	#print('Starting: ', a)
	#print('Dealer:', b[0], '*')
	mon = -len(a) * 10

	dub = []
	for j in range(len(a)):
		dub.append(False)
	index = -1
	for hand in a:
		#print('')
		#print('Playing with', hand2str(hand), '(', getTotalValue(hand), ')')
		ag = False
		index += 1
		while (testSplit(hand, b)):
			a.append([hand[0], getCard()])
			hand[1] = getCard()
			dub.append(False)
			mon += -10
			#print('split' + (' again' if ag else ''))
			#print(a)
			ag = True

		s = getTotalValue(hand)
		if (s == 9 and b[0] >= 3 and b[0] <= 6) or (s == 10 and b[0] >= 2 and b[0] <= 9) or (s == 11 and b[0] != 1):
			hand.append(getCard())
			dub[index] = True
			mon += -10
			#print('Double', hand2str(hand), '(', getTotalValue(hand), ')')
			continue

		while True:
			if (s <= 11) or (s == 12 and not (b[0] >= 4 and b[0] <= 6)) or (s >= 13 and s <= 16 and not (b[0] >= 2 and b[0] <= 6)):
				hand.append(getCard())
				#print('Hit', hand2str(hand), '(', getTotalValue(hand), ')')
				s = getTotalValue(hand)
			else:
				#print('Stand')
				break

	#print('')
	s = getTotalValue(b)
	while (s <= 16):
		b.append(getCard())
		s = getTotalValue(b)
		#print('Dealer hit', hand2str(b), '(', s, ')')

	#print('Final', a)
	#print('Dealer', hand2str(b), '(', s, ')')

	win = 0
	draw = 0
	for j in range(len(a)):
		hand = a[j]
		if (getTotalValue(hand) <= 21):
			if (getTotalValue(b) > 21) or (getTotalValue(hand) > getTotalValue(b)):
				win += 1
				mon += 20
				if (dub[j]):
					mon += 40
				if (len(hand) == 2) and (getTotalValue(hand) == 21):
					mon += 5
			elif (getTotalValue(hand) == getTotalValue(b)):
				mon += 10
				draw += 1
				if (dub[j]):
					mon += 10
				if (len(hand) == 2 and len(b) > 2) and (getTotalValue(hand) == 21):
					mon += 15
				#print('- - - - -')
				#print(hand2str(hand), '(', getTotalValue(hand), ')')
				#print('D:', hand2str(b), '(', getTotalValue(b), ')')


	return win, len(a), mon, draw

def getStat(a, b, short=False):
	w = 0
	g = 0
	m = 0
	d = 0
	GAMES = 100000
	for i in range(GAMES):
		tw, tg, tm, td = play([a], [b, getCard()])
		w += tw
		g += tg
		m += tm
		d += td
	if not short:
		print('======================')
		print('Hands:', g)
		print('Wins:', w)
		print('Draws:', d)
		print('Win chance:', round(w/g*100, 2), '%')
		print('Draw chance', round(d/g*100, 2), '%')
		print('Money:', m)
		print('Money AVG:', round(m/GAMES, 2))
	else:
		return 'W: ' + str(round(w/g*100, 2)) + '% D: ' + str(round(d/g*100, 2)) + '% MA: ' + str(round(m/GAMES, 2))
#print('Stats:')
#for i in range(14):
#	print((cards[i] if (i in cards) else str(i)) + ' =', round(stat[i]/gstat, 3)*100, '%')

player = input('Enter your cards:').split()
for i in range(len(player)):
	player[i] = bcards[player[i]] if (player[i] in bcards) else int(player[i])
print(player)
dealer = input('Enter dealer card:')
dealer = bcards[dealer] if (dealer in bcards) else int(dealer)
print(dealer)
getStat(player, dealer)