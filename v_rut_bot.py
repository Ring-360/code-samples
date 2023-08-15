#
# Телеграм-бот с мини-тестом из 5-ти вопросов.
# При "правильном" прохождении теста выдавался определённый результат, в остальных случаях - один из заготовленных.
# Также бот записывает статистику ответов пользователей в БД для последующего анализа в целях оптимизации рекламной кампании.
#

import logging

import aiogram.utils.markdown as md
from aiogram import Bot, Dispatcher, types
from aiogram.contrib.fsm_storage.memory import MemoryStorage
from aiogram.contrib.fsm_storage.mongo import MongoStorage
from aiogram.dispatcher import FSMContext
from aiogram.dispatcher.filters import Text
from aiogram.dispatcher.filters.state import State, StatesGroup
from aiogram.types import ParseMode
from aiogram.utils import executor
import random

logging.basicConfig(level=logging.INFO)

API_TOKEN =''

bot = Bot(token=API_TOKEN)

# For example use simple MemoryStorage for Dispatcher.
#storage = MemoryStorage()
storage = MongoStorage(host='localhost', port=27017, db_name='v_rut_bot')
dp = Dispatcher(bot, storage=storage)

import motor.motor_asyncio
client = motor.motor_asyncio.AsyncIOMotorClient('localhost', 27017)
db = client.v_rut_bot

answers = dict()
answers['_1'] = {'Иван' : [True, 1], 'Лев' : [False, 2], 'Леонид' : [False, 3]}
answers['_2'] = {'Начальником ж/д станции' : [False, 1],
				'Сцена ДК РУТ (МИИТ) для меня только начало' : [True, 2],
				'О чём вы? Я даже не знаю, какие пары будут завтра' : [False, 3]}
answers['_3'] = {'с Анатолием Шапашниковым' : [False, 1], 'с Митей Алмазовым' : [True, 2], 'с Григорием Лепсем' : [False, 3]}
answers['_4'] = {'Конечно ведущий, мне до него ещё расти и расти' : [True, 1],
				'Конечно я, зумеры выкупают мои рофлы' : [False, 2],
				'Скажем так: шутки моих преподавателей смешнее' : [False, 3]}
answers['_5'] = {'Чай, Индийский чай' : [False, 1], 'Кофе — и точка.' : [False, 2], 'Молочный коктейль из «Эдисон Экспресс»' : [True, 3]}

images = dict()
images['_1'] = 'AgACAgIAAxkDAAICNGQHf8zgAeIR7of0VyDJ4kjLk8V1AAI1xjEbdyhBSD_PdKOOQxVIAQADAgADeQADLgQ'
images['_2'] = 'AgACAgIAAxkDAAICPmQHgNrV9FfAkEPgd2yolcCOB24JAAI9xjEbdyhBSO0z3dtA8biwAQADAgADeQADLgQ'
images['_3'] = 'AgACAgIAAxkDAAICQGQHgPnoJfuIXuMHFqyguiHMeDIyAAI-xjEbdyhBSOQPtR0fa4OXAQADAgADeQADLgQ'
images['_4'] = 'AgACAgIAAxkDAAICQmQHgQoFp5PcKvp9RfpDnd_jGWNcAAI_xjEbdyhBSMM4cnkyQQX8AQADAgADeQADLgQ'
images['_5'] = 'AgACAgIAAxkDAAICRGQHgR6Hu4wWsp5X6cXX53_P_7gdAAJAxjEbdyhBSGpcqjiFRN8QAQADAgADeQADLgQ'

class test(StatesGroup):
	_1 = State()
	_2 = State()
	_3 = State()
	_4 = State()
	_5 = State()

async def isRight(message: types.Message, state: FSMContext):
	state_name = (await state.get_state())[5:]
	if message.text in answers[state_name]:
		await db.answers.update_one({'tgid' : message.chat.id}, {'$set' : {state_name : answers[state_name][message.text][1]}}, upsert=True)
		if not answers[state_name][message.text][0]:
			await state.update_data(test=False)
		return True
	else:
		await message.answer('Неизвестный ответ\n\nВыбери ответ из представленных снизу')
		return False

@dp.message_handler(commands='start', state='*')
async def init(message: types.Message, state: FSMContext):
	await test._1.set()
	await state.update_data(test=True)
	markup = types.ReplyKeyboardMarkup(row_width=1, resize_keyboard=True)
	for key in answers['_1']:
		markup.insert(types.KeyboardButton(text=key))
	await message.answer_photo(images['_1'], caption='<b>Вопрос 1/5</b>\n\nПривет!\nДавай узнаем, сможешь ли ты стать гостем на шоу «Вечерний РУТ»!\n\nНачнём...\n\nКак зовут ведущего шоу?', reply_markup=markup, parse_mode='HTML')
	#logging.info(ans)
	#await message.answer_photo(open('./v_rut_bot/TG_vopros_1.jpg', 'rb'))

@dp.message_handler(state=test._1)
async def answer_1(message: types.Message, state: FSMContext):
	if not (await isRight(message, state)):
		return
	await state.update_data(first=answers['_1'][message.text][0])
	await test._2.set()
	markup = types.ReplyKeyboardMarkup(row_width=1, resize_keyboard=True)
	for key in answers['_2']:
		markup.insert(types.KeyboardButton(text=key))
	await message.answer_photo(images['_2'], caption='<b>Вопрос 2/5</b>\n\nГде ты видишь себя через 10 лет?', reply_markup=markup, parse_mode='HTML')
	#logging.info(ans)

@dp.message_handler(state=test._2)
async def answer_2(message: types.Message, state: FSMContext):
	if not (await isRight(message, state)):
		return
	await test._3.set()
	markup = types.ReplyKeyboardMarkup(row_width=1, resize_keyboard=True)
	for key in answers['_3']:
		markup.insert(types.KeyboardButton(text=key))
	await message.answer_photo(images['_3'], caption='<b>Вопрос 3/5</b>\n\nСкоро лето, с кем хочешь отправиться в отпуск?', reply_markup=markup, parse_mode='HTML')
	#logging.info(ans)

@dp.message_handler(state=test._3)
async def answer_3(message: types.Message, state: FSMContext):
	if not (await isRight(message, state)):
		return
	await test._4.set()
	markup = types.ReplyKeyboardMarkup(row_width=1, resize_keyboard=True)
	for key in answers['_4']:
		markup.insert(types.KeyboardButton(text=key))
	await message.answer_photo(images['_4'], caption='<b>Вопрос 4/5</b>\n\nКто смешнее, ты или ведущий?', reply_markup=markup, parse_mode='HTML')
	#logging.info(ans)

@dp.message_handler(state=test._4)
async def answer_4(message: types.Message, state: FSMContext):
	if not (await isRight(message, state)):
		return
	await test._5.set()
	markup = types.ReplyKeyboardMarkup(row_width=1, resize_keyboard=True)
	for key in answers['_5']:
		markup.insert(types.KeyboardButton(text=key))
	await message.answer_photo(images['_5'], caption='<b>Вопрос 5/5</b>\n\nВозможно ты замечал, что у каждого звёздного гостя шоу на столе стоит чашка с напитком.\
\n\nЧто попросишь налить себе, если тебя пригласят?', reply_markup=markup, parse_mode='HTML')
	#logging.info(ans)

@dp.message_handler(state=test._5)
async def answer_5(message: types.Message, state: FSMContext):
	if not (await isRight(message, state)):
		return
	data = await state.get_data()

	msg = '<b>Результат</b>\n\n'
	photo = ''

	data = await state.get_data()
	if data['test'] == True:
		msg += 'Поздравляем!\nУ тебя есть все шансы стать следующим гостем шоу «Вечерний РУТ».'
		photo = 'AgACAgIAAxkDAAICXWQHgdMbtl1aAu1dsiu6qBEmFb9AAAJExjEbdyhBSETJ3GZgxWjxAQADAgADeQADLgQ'
	elif data['first'] == False:
		msg += 'Извини, но для начала тебе стоит узнать имя ведущего :)'
		photo = 'AgACAgIAAxkDAAICamQHgf6dKV2cBeHIfMZO-2AMcJaeAAJhxjEbdyhBSF8x6k_GNSZGAQADAgADeQADLgQ'
	else:
		if random.randint(0,1) == 0:
			msg += 'Знаешь, в зрительном зале кресла удобнее)'
			photo = 'AgACAgIAAxkDAAIChGQHgkrBuGE7L0ETdZnOM-1kbDDqAAJBxjEbdyhBSPOH3uByBxBNAQADAgADeQADLgQ'
		else:
			msg += 'Увы, тебе лучше заняться учёбой, но на шоу тоже приходи, только в качестве зрителя)'
			photo = 'AgACAgIAAxkDAAICzGQHoBV63rc_9XeIN39UiUjISdA2AALSxjEbdyhBSLUfUxZMB8xcAQADAgADeQADLgQ'
	msg_new = '\n\nКстати, надеюсь, ты знаешь, что первый эфир шоу «Вечерний РУТ» состоится <b>13 марта в 11:00</b> во Дворце Культуры РУТ (МИИТ). \
\nУспей занять место в первом ряду!'

	await message.answer_photo(photo,caption=msg, parse_mode='HTML')
	#logging.info(ans)
	markup = types.ReplyKeyboardMarkup(row_width=1, resize_keyboard=True)
	markup.insert(types.KeyboardButton(text='Пройти тест ещё раз'))
	await message.answer(msg_new, reply_markup=markup, parse_mode='HTML')
	await state.finish()

@dp.message_handler(state='*')
async def again(message: types.Message, state: FSMContext):
	if message.text == 'Пройти тест ещё раз':
		await init(message, state)

if __name__ == '__main__':
    executor.start_polling(dp, skip_updates=True)