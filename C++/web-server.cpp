/**

	Веб-сервер с движком одного из давних проектов.
	Данный код появился в результате моего оооочень большого нежелания учить PHP в своё время. (а кроме php тогда ничего и не было)
	Поэтому было принято решение писать на плюсах одновременно и веб-сервер, и движок сайта с API, глубоко интегрировав их друг с другом в едином коде.
	В данном коде в основном движок, обрабатывающий http-запросы и реализующий часть внутренней логики.
	Часть, относящуюся непосредственно к веб-серверу не нашёл, но там был опыт работы с потоками, мьютексами, сокетами, TLS-шифрованием и TCP/IP стеком впринципе.

**/


#include "requestHandler.h"
#include <map>
#include <sstream>
#include <openssl/sha.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fstream>
namespace http = boost::beast::http;
using namespace std;

string copy(string s, int n, int k) {
	string r = "";
	for (int i = n; i < (n + k); i++) {
		r = r + s[i];
	}
	return r;
}

string s_copy(string &s, int n, int k) {
	string r = "";
	unsigned int size = s.size();
	for (int i = n; i < (n + k); i++) {
		if (i < size) {
			r += s[i];
		}
		else {
			return r;
		}
	}
	return r;
}

string s_copy_sql(string &s, int n, int k) {
	string r = "";
	unsigned int size = s.size();
	for (int i = n; i < (n + k); i++) {
		if (i < size) {
			if (s[i] == '\'' || s[i] == '\\') {
				return r;
			}
			r += s[i];
		}
		else {
			return r;
		}
	}
	return r;
}

class parser {
private:
	string str, res, content;
	int i, size;
	bool space, stop, waitingArg;
	unsigned int waitingTagID, line, place;

	void parseHeader() {
		while (i < size) {
			if (!stop) {
				if (str[i] == '{') {
					res += "<br>";
					setTag(0);
					parseCode();
				}
				else if (str[i] == ' ') {
					if (!space) {
						res += " ";
						content += " ";
						space = true;
					}
				}
				else if (str[i] == '\\') {
					stop = true;
				}
				else if (str[i] == '}') {
					throw (string)"Syntax error: block not opened";
				}
				else if (str[i] == '#') {
					if (testTag() != 0) {
						throw (string)"Grammar error: tag cannot be in header";
					}
					else {
						res += str[i];
						content += str[i];
					}
				}
				else if (str[i] == '\n') { line++; place = 1; }
				else {
					if (space) {
						space = false;
					}
					res += str[i];
					content += str[i];
				}
			}
			else {
				res += str[i];
				content += str[i];
				stop = false;
			}
			i++;
			place++;
		}
		return;
	}

	void parseCode() {
		parseCode(0);
	}

	void parseCode(unsigned int currentTagID) {
		i++;
		waitingTagID = 0;
		unsigned int openedBrackets = 0;
		while (i < size) {
			if (!stop) {
				if (str[i] == '{') {
					if (waitingArg) {
						throw "Syntax error: tag " + resolveTag(waitingTagID) + " require argument";
					}
					else if (waitingTagID != 0) {
						parseCode(waitingTagID);
					}
					else {
						openedBrackets++;
					}
				}
				else if (str[i] == '}') {
					if (waitingArg) {
						throw "Syntax error: tag " + resolveTag(waitingTagID) + " require argument";
					}
					else if (waitingTagID != 0) {
						throw "Syntax error: tag " + resolveTag(waitingTagID) + " require a body";
					}
					else if (openedBrackets > 0) {
						openedBrackets--;
					}
					else {
						break;
					}
				}
				else if (str[i] == '\n') { line++; place = 1; }
				else if (str[i] == '\\') {
					stop = true;
				}
				else if (str[i] == ' ') {
					if (!space && !waitingArg) {
						res += str[i];
						content += str[i];
						space = true;
					}
				}
				else if (str[i] == '#') {
					if (waitingArg) {
						throw "Syntax error: tag " + resolveTag(waitingTagID) + " require argument";
					}
					if (waitingTagID != 0) {
						throw "Syntax error: tag " + resolveTag(waitingTagID) + " require block";
					}
					unsigned int posTagID = testTag();
					if (posTagID != 0) {
						if (currentTagID == 0 || (currentTagID == 1 && posTagID == 3) || (currentTagID == 3 && posTagID == 1)
							|| (currentTagID == 4 && posTagID == 1) || (currentTagID == 4 && posTagID == 3)) {
							setTag(posTagID);
						}
						else {
							throw "Grammar error: not allowed to put " + resolveTag(posTagID) + " into " + resolveTag(currentTagID);
						}
					}
					if (posTagID == 1 || posTagID == 2 || posTagID == 4) {
						i += 3;
						place += 3;
					}
					else if (posTagID == 3 || posTagID == 5) {
						i += 4;
						place += 4;
					}
					if (posTagID == 1 || posTagID == 2) {
						waitingArg = true;
						waitingTagID = posTagID;
					}
					else if (posTagID >= 3 && posTagID <= 5) {
						waitingTagID = posTagID;
					}
					else {
						res += str[i];
						content += str[i];
						if (space) {
							space = false;
						}
					}
				}
				else if (str[i] == '(') {
					if (waitingArg) {
						waitingArg = false;
						i++;
						place++;
						parseArg();
						res += "\">";
					}
					else {
						if (waitingTagID != 0) {
							if (isArgumentTag(waitingTagID)) {
								throw "Syntax error: tag " + resolveTag(waitingTagID) + " require block";
							}
							else {
								throw "Syntax error: tag " + resolveTag(waitingTagID) + " does not require argument";
							}
						}
						else
						{
							res += str[i];
							content += str[i];
						}
					}
				}
				else {
					if (waitingArg) {
						throw "Syntax error: tag " + resolveTag(waitingTagID) + " require argument";
					}
					else if (waitingTagID != 0) {
						throw "Syntax error: tag " + resolveTag(waitingTagID) + " require a block";
					}
					res += str[i];
					content += str[i];
					if (space) {
						space = false;
					}
				}
			}
			else {
				if (waitingArg) {
					throw "Syntax error: tag " + resolveTag(waitingTagID) + " require argument";
				}
				else if (waitingTagID != 0) {
					throw "Syntax error: tag " + resolveTag(waitingTagID) + " require block";
				}
				if (str[i] == 'n') {
					res += "<br>";
				}
				else {
					res += str[i];
					content += str[i];
				}
				stop = false;
			}
			i++;
			place++;
		}
		if (i >= size) {
			throw (string)"Syntax error: block not closed, expected '}'";
		}
		closeTag(currentTagID);
		return;
	}

	void parseArg() {
		while (i < size) {
			if (!stop) {
				if (str[i] == '{' || str[i] == '}' || str[i] == '(') {
					throw (string)"Syntax error: argument must contain only required string";
				}
				else if (str[i] == ')') {
					break;
				}
				else if (str[i] == '\n') { line++; place = 1; }
				else if (str[i] == '\\') {
					stop = true;
				}
				else if (str[i] == ' ') {}
				else {
					res += str[i];
				}
			}
			else {
				if (str[i] == 'n') {
					throw (string)"Syntax error: argument must contain only required string";
				}
				else {
					res += str[i];
				}
				stop = false;
			}
			i++;
			place++;
		}
		if (i >= size) {
			throw "Syntax error: tag " + resolveTag(waitingTagID) + " require argument;\nSyntax error: argument brackets not closed, expected ')'";
		}
		return;
	}

	unsigned int testTag() {
		if (i + 3 < size) {
			string posTag = copy(str, i, 4);
			if (i + 4 < size) {
				if (copy(str, i, 5) == "#bold") {
					return 3;
				}
				else if (copy(str, i, 5) == "#warn") {
					return 5;
				}
			}
			if (posTag == "#ref") {
				return 1;
			}
			else if (posTag == "#img") {
				return 2;
			}
			else if (posTag == "#inf") {
				return 4;
			}
			else {
				return 0;
			}
		}
		else {
			return 0;
		}
	}

	string resolveTag(unsigned int id) {
		switch (id) {
		case 1: return "#ref";
		case 2: return "#img";
		case 3: return "#bold";
		case 4: return "#inf";
		case 5: return "#warn";
		default: return "not found";
		}
	}

	void setTag() {
		setTag(0);
	}

	void setTag(unsigned int tagID) {
		switch (tagID) {
		case 0: {
			res += "<div class=\"block\">";
			return;
		}
		case 1: {
			res += "<a href=\"";
			return;
		}
		case 2: {
			res += "<img src=\"";
			return;
		}
		case 3: {
			res += "<span class=\"bold\">";
			return;
		}
		case 4: {
			res += "<div class=\"inf\">";
			return;
		}
		case 5: {
			res += "<div class=\"warn\">";
			return;
		}
		}
	}

	void closeTag(unsigned int tagID) {
		switch (tagID) {
		case 0: {res += "</div>"; return; }
		case 1: {res += "</a>"; return; }
		case 2: {return; }
		case 3: {res += "</span>"; return; }
		case 4: {res += "</div>"; return; }
		case 5: {res += "</div>"; return; }
		}
	}

	bool isArgumentTag(unsigned int tagID) {
		if (tagID == 1 || tagID == 2) {
			return true;
		}
		else {
			return false;
		}
	}


public:
	parser() : str(""), res(""), i(0), line(1), place(1), size(0), space(false), stop(false), waitingArg(false), waitingTagID(0), content("") {}

	bool run(string &input, string &output, string &cont) {
		try {
			str = input;
			size = str.size();
			if (size < 1) {
				throw (string)"Empty code";
			}
			parseHeader();
			output = res;
			cont = content;
			return true;
		}
		catch (string &e) {
			output = e + " [line " + to_string(line) + "; column " + to_string(place) + "]";
			return false;
		}
	}
};

class ResultSet {
public:
	boost::scoped_ptr<sql::ResultSet> res;
	ResultSet(sql::ResultSet* resptr) : res(resptr) {}
	ResultSet() {}
	void operator=(sql::ResultSet* resptr) { res.reset(resptr); }
	string getString(string column) {
		return (string)res->getString(column.c_str()).c_str();
	}
};

class sqlRequest {
	string req;
public:
	sqlRequest() : req("") {}
	void operator=(string str) { req = str; }
	void operator+=(string str) { req += str; }
	operator sql::SQLString() { return req.c_str(); }
};

bool isInVector(char item, const string &values) {
	for (auto now : values) {
		if (item == now) {
			return true;
		}
	}
	return false;
}

bool isInVector(string item, const vector<string> &values) {
	item = boost::to_lower_copy(item);
	for (auto now : values) {
		if (item == now) {
			return true;
		}
	}
	return false;
}

bool isAllInVector(string &str, const string &values) {
	if (str == "") {
		return false;
	}
	bool isIn = false;
	for (auto item : str) {
		for (auto now : values) {
			if (item == now) {
				isIn = true;
				break;
			}
		}
		if (!isIn) {
			return false;
		}
		isIn = false;
	}
	return true;
}

bool isNotAnyInVector(string &str, const string &values) {
	for (auto item : str) {
		for (auto now : values) {
			if (item == now) {
				return false;
			}
		}
	}
	return true;
}

bool isHex(string str) {
	str = boost::to_upper_copy(str);
	const string hexSymbols = "0123456789ABCDEF";
	return isAllInVector(str, hexSymbols);
}

bool isPass(string str) {
	if (str.size() > 128 || str == "" || str.size() < 8) {
		return false;
	}
	str = boost::to_lower_copy(str);
	const string passSymbols = "1234567890abcdefghijklmnopqrstuvwxyz!@#$%^&*_=+*-";
	return isAllInVector(str, passSymbols);
}

bool isName(string str) {
	if (str.size() > 32 || str == "") {
		return false;
	}
	const string restrictedSymbols = " `~!@#$%^&*()-_=+|\\/*.,<>\":;/?'{}[]";
	return isNotAnyInVector(str, restrictedSymbols);
}

bool isEmail(string str) {
	if (str.size() > 128 || str == "") {
		return false;
	}
	const string emailSymbols = "1234567890abcdefghijklmnopqrstuvwxyz_.@-";
	if (!isAllInVector(str, emailSymbols)) {
		return false;
	}
	bool isMail = false, isDomain = false;
	for (auto now : str) {
		if (now == '@') {
			if (!isMail) {
				isMail = true;
			}
			else {
				return false;
			}
		}
		else if (now == '.') {
			if (!isDomain && isMail) {
				isDomain = true;
			}
		}
	}
	if (isMail && isDomain) {
		return true;
	}
	else {
		return false;
	}
}

bool isDate(string &str) {
	if (str.size() != 10) {
		return false;
	}
	const string numbers = "0123456789";
	for (int i = 0; i < 10; i++) {
		if (i == 4 || i == 7) {
			if (str[i] != '-') {
				return false;
			}
		}
		else {
			if (!isInVector(str[i], numbers)) {
				return false;
			}
		}
	}
	return true;
}

bool isTitle(string &str) {
	if (str.size() > 64 || str.size() == 0) {
		return false;
	}
	const string restrictedSymbols = "`~!@#$%^&*-_=+|\\/*.,<>\":;/?'{}[]";
	return isNotAnyInVector(str, restrictedSymbols);
}

bool isExists(string &req, sql::Statement* stmt) {
	ResultSet res = stmt->executeQuery(req.c_str());
	if (res.res->next()) {
		return true;
	}
	else {
		return false;
	}
}

bool isUUID(string &str) {
	if (str.size() != 36) {
		return false;
	}
	const string hexSymbols = "0123456789abcdef";
	for (int i = 0; i < 36; i++) {
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			if (str[i] != '-') {
				return false;
			}
		}
		else {
			if (!isInVector(str[i], hexSymbols)) {
				return false;
			}
		}
	}
	return true;
}

bool isUniqueUUID(string &uuid, sql::Statement* stmt) {
	string req = "select uuid from sessions where uuid = '" + uuid + "' limit 1";
	return !isExists(req, stmt);
}

bool isNumber(string &str) {
	if (str == "") {
		return false;
	}
	const string nums = "0123456789";
	for (auto now : str) {
		if (!isInVector(now, nums)) {
			return false;
		}
	}
	return true;
}

bool isArticleExists(string &id, sql::Statement* stmt) {
	if (!isNumber(id)) {
		return false;
	}
	string req = "select id from articles where id = " + id + " limit 1";
	return isExists(req, stmt);
}

bool isLang(string &str) {
	const vector<string> langs = { "ru" };
	return isInVector(str, langs);
}


bool getArticleIfExistsByTitle(string &title, string &lang, sql::Statement* stmt, ResultSet &res) {
	if (!isTitle(title) || !isLang(lang)) { return false; }
	string req = "select html from articles where title = '" + title + "' and lang = '" + lang + "' limit 1";
	res = stmt->executeQuery(req.c_str());
	if (res.res->next()) {
		return true;
	}
	else {
		return false;
	}
}

bool isCategory(string &str, sql::Statement* stmt) {
	if (!isNumber(str)) {
		return false;
	}
	string req = "select id from category where id = " + str + " limit 1";
	return isExists(req, stmt);
}

string decodeURI(string &str) {
	string res = "";
	unsigned int i = 0;
	while (i < str.size()) {
		if (str[i] == '%') {
			if (i + 2 < str.size()) {
				if (isHex(copy(str, i + 1, 2))) {
					res += (char)stoul(copy(str, i + 1, 2), nullptr, 16);
					i += 2;
				}
				else {
					res += str[i];
				}
			}
			else {
				res += str[i];
			}
		}
		else if (str[i] == '+') {
			res += ' ';
		}
		else {
			res += str[i];
		}
		i++;
	}
	return res;
}

map<string, string> parseCookies(string cookies) {
	const vector<string> posValues = { "uuid", "lang" };
	unsigned int i = 0;
	bool onName = true;
	map<string, string> cookiesMap;
	string name = "", value = "";
	while (i < cookies.size()) {
		if (cookies[i] == '=') {
			if (onName) {
				onName = false;
			}
			else {
				return cookiesMap;
			}
		}
		else if (cookies[i] == ';') {
			if (onName) {
				return cookiesMap;
			}
			else {
				if (isInVector(name, posValues)) {
					cookiesMap[boost::to_lower_copy(name)] = value;
				}
				name = "";
				value = "";
				onName = true;
			}
		}
		else if (cookies[i] == ' ') {}
		else {
			if (onName) {
				name += cookies[i];
			}
			else {
				value += cookies[i];
			}
		}
		i++;
	}
	if (!onName && isInVector(name, posValues)) {
		cookiesMap[boost::to_lower_copy(name)] = value;
	}
	return cookiesMap;
}

map<string, string> parseParam(string str, const vector<string> &posValues) {
	map<string, string> paramMap;
	string name = "", value = "";
	unsigned int i = 0;
	bool stop = false, onName = true;
	while (i < str.size()) {
		if (!stop) {
			if (str[i] == '=') {
				if (onName) {
					onName = false;
				}
				else {
					return paramMap;
				}
			}
			else if (str[i] == '&') {
				if (onName) {
					return paramMap;
				}
				else {
					if (isInVector(name, posValues)) {
						paramMap[boost::to_lower_copy(name)] = decodeURI(value);
					}
					name = "";
					value = "";
					onName = true;
				}
			}
			else if (str[i] == '\\') {
				stop = true;
			}
			else {
				if (onName) {
					name += str[i];
				}
				else {
					value += str[i];
				}
			}
		}
		else {
			stop = false;
			if (onName) {
				name += str[i];
			}
			else {
				value += str[i];
			}
		}
		i++;
	}
	if (!onName) {
		if (isInVector(name, posValues)) {
			paramMap[boost::to_lower_copy(name)] = value;
		}
	}
	return paramMap;
}

map<string, string> parseTarget(string targetURI, string &target) {
	targetURI = decodeURI(targetURI);
	map<string, string> queryMap;
	const vector<string> posValues = { "ref", "q", "offset", "live" };
	string name = "", value = "";
	target = "";
	unsigned int i = 0;
	bool onName = false, onTarget = true;
	while (i < targetURI.size()) {
		if (targetURI[i] == '?') {
			if (onTarget) {
				onTarget = false;
				onName = true;
			}
			else {
				return queryMap;
			}
		}
		else if (targetURI[i] == '=') {
			if (!onTarget && onName) {
				onName = false;
			}
			else {
				return queryMap;
			}
		}
		else if (targetURI[i] == '&') {
			if (!onTarget && !onName) {
				if (isInVector(name, posValues)) {
					queryMap[boost::to_lower_copy(name)] = value;
				}
				name = "";
				value = "";
				onName = true;
			}
			else {
				return queryMap;
			}
		}
		else if (targetURI[i] == '#') {
			return queryMap;
		}
		else {
			if (onTarget) {
				if (targetURI[i] == '/') {
					if (i + 1 < targetURI.size()) {
						if (!(targetURI[i + 1] == '?' || targetURI[i + 1] == '#')) {
							target += targetURI[i];
						}
					}
				}
				else {
					target += targetURI[i];
				}
			}
			else {
				if (onName) {
					name += targetURI[i];
				}
				else {
					value += targetURI[i];
				}
			}
		}
		i++;
	}
	if (!onName && !onTarget) {
		if (isInVector(name, posValues)) {
			queryMap[boost::to_lower_copy(name)] = value;
		}
	}
	return queryMap;
}

string generateUUID() {
	try {
		boost::uuids::uuid uuid = boost::uuids::random_generator()();
		return boost::uuids::to_string(uuid);
	}
	catch (boost::uuids::entropy_error e) {
		throw 4;
	}
}

string generateSalt() {
	string uuid = generateUUID(), salt = "";
	for (auto now : uuid) {
		if (now != '-') {
			salt += now;
		}
	}
	return salt;
}

string passSha512(string &pass, string &salt) {
	unsigned char hash[SHA512_DIGEST_LENGTH];
	char* str = _strdup((pass + salt).c_str());

	SHA512_CTX ctx;
	SHA512_Init(&ctx);
	SHA512_Update(&ctx, str, strlen(str));
	SHA512_Final(hash, &ctx);

	char hash_string[SHA512_DIGEST_LENGTH * 2 + 1];
	for (int i = 0; i < SHA512_DIGEST_LENGTH; i++) {
		sprintf_s(&hash_string[i * 2], 3, "%02x", (unsigned int)hash[i]);
	}

	free(str);
	return hash_string;
}

unsigned int registerUser(map<string, string> &params, sqlRequest &sql) {
	sql = "insert into accounts(name,lname,passHash,passSalt,email,birthday) values (";

	if (isName(params["name"])) {
		sql += "'" + params["name"] + "',";
	}
	else {
		return 3;
	}

	if (isName(params["lname"])) {
		sql += "'" + params["lname"] + "',";
	}
	else {
		return 4;
	}

	if (isPass(params["pass"])) {
		string salt = generateSalt() + generateSalt();
		sql += "'" + passSha512(params["pass"], salt) + "',";
		sql += "'" + salt + "',";
	}
	else {
		return 5;
	}

	sql += "'" + params["mail"] + "',";

	if (isDate(params["birth"])) {
		sql += "'" + params["birth"] + "')";
	}
	else {
		return 6;
	}

	return 0;
}

bool isUserLoggedIn(string &uuid, sql::Statement* stmt) {
	if (!isUUID(uuid)) {
		return false;
	}
	string sqlReq = "select id from sessions where uuid = '" + uuid + "' limit 1";
	return isExists(sqlReq, stmt);
}

string resolveUser(int id, sql::Statement* stmt) {
	string req = "select name from accounts where id = " + to_string(id) + " limit 1";
	ResultSet res = stmt->executeQuery(req.c_str());
	res.res->next();
	string name = res.getString("name");
	return name;
}

string resolveUserByUUID(string &uuid, sql::Statement* stmt) {
	string req = "select id from sessions where uuid = '" + uuid + "' limit 1";
	ResultSet res = stmt->executeQuery(req.c_str());
	res.res->next();
	int id = res.res->getInt("id");
	return resolveUser(id, stmt);
}

unsigned int loginUser(map<string, string> &bodyParams, sql::Statement* stmt, string &uuid) {
	string sqlReq = "";
	ResultSet sqlRes;
	if (isEmail(bodyParams["mail"]) && isPass(bodyParams["pass"])) {
		sqlReq = "select id, passHash, passSalt from accounts where email = '" + bodyParams["mail"] + "' limit 1";
		sqlRes = stmt->executeQuery(sqlReq.c_str());
		if (sqlRes.res->next()) {
			string salt = sqlRes.getString("passSalt");
			if (passSha512(bodyParams["pass"], salt) == sqlRes.getString("passHash")) {
				uuid = generateUUID();
				unsigned int times = 0;
				while (!isUniqueUUID(uuid, stmt)) {
					if (times > 1000) {
						throw 4;
					}
					uuid = generateUUID();
					times++;
				}
				sqlReq = "insert into sessions(uuid, id) values ('" + uuid + "'," + to_string(sqlRes.res->getInt("id")) + ")";
				stmt->execute(sqlReq.c_str());
				return 0;
			}
			else {
				return 1;
			}
		}
		else {
			return 1;
		}
	}
	else {
		return 1;
	}
}

string sqlGuard(string &str) {
	string res = "";
	for (auto now : str) {
		if (now == '\'') {
			res += "''";
		}
		else if (now == '\\') {
			res += "\\\\";
		}
		else if (now == '<') {
			res += "&lt";
		}
		else if (now == '>') {
			res += "&gt";
		}
		else if (now == '&') {
			res += "&amp";
		}
		else {
			res += now;
		}
	}
	return res;
}

string htmlGuard(string &str) {
	string res = "";
	for (auto now : str) {
		if (now == '<') {
			res += "&lt";
		}
		else if (now == '>') {
			res += "&gt";
		}
		else if (now == '&') {
			res += "&amp";
		}
		else {
			res += now;
		}
	}
	return res;
}

vector<string> getContent(sql::ResultSet* res) {
	vector<string> result;
	while (res->next()) {
		result.push_back((string)res->getString("content").c_str());
	}
	return result;
}

unsigned int createArticle(map<string, string> &bodyParams, sql::Statement* stmt, string &lang, string &msg) {
	string req = "";
	ResultSet res;

	if (!isTitle(bodyParams["title"])) { return 6; }

	if (!isLang(lang)) {
		return 3;
	}
	else {
		lang = boost::to_lower_copy(lang);
	}

	req = "select id from articles where title = '" + bodyParams["title"] + "' and lang = '" + lang + "' limit 1";
	if (isExists(req, stmt)) {
		return 2;
	}

	if (isArticleExists(bodyParams["ref"], stmt)) {
		req = "select id from articles where ref = " + bodyParams["ref"] + " and lang = '" + lang + "' limit 1";
		if (isExists(req, stmt)) {
			return 1;
		}
		req = "select category from articles where id = " + bodyParams["ref"] + " limit 1";
		res = stmt->executeQuery(req.c_str());
		res.res->next();
		bodyParams["category"] = res.getString("category");
	}
	else {
		if (!isCategory(bodyParams["category"], stmt)) {
			return 5;
		}
		bodyParams["ref"] = "0";
	}

	parser p;
	string content = "";
	if (p.run(bodyParams["code"], msg, content)) {
		req = "insert into articles(title, code, html, category, lang, ref, content) values (";
		req += "'" + sqlGuard(bodyParams["title"]) + "',";
		req += "'" + sqlGuard(bodyParams["code"]) + "',";
		req += "'" + sqlGuard(msg) + "',";
		req += bodyParams["category"] + ",";
		req += "'" + lang + "',";
		req += bodyParams["ref"] + ",";
		req += "'" + sqlGuard(content) + "')";
		stmt->execute(req.c_str());
		return 0;
	}
	else {
		return 4;
	}

}

string prepareSearchString(string &str, string &regexp) {
	string res = "";
	regexp = "(";
	bool space = false, was = false;
	const string restrictedSymbols = "`~!@#$%^&*()-_=+|\\/*.,<>\":;/?'{}[]";
	for (auto now : str) {
		if (now == ' ') {
			if (!space && was) {
				res += " ";
				space = true;
			}
		}
		else if (!isInVector(now, restrictedSymbols)) {
			res += now;
			if (space && was) {
				space = false;
				regexp += "|";
				regexp += now;
			}
			else {
				regexp += now;
			}
			if (!was) {
				was = true;
			}
		}
	}
	regexp += ")";
	return res;
}

http::response<http::string_body>* handleRequest(http::request<http::dynamic_body>* req, sql::Statement* stmt) {
	const vector<string> methods = { "get", "post", "head" };
	http::response<http::string_body>* res = new http::response<http::string_body>;
	res->version(11);
	res->set(http::field::server, "Cube Web Server");
	map<string, string> cookies, params;
	string target = "", method = "", host = "";
	sqlRequest sqlReq;
	ResultSet sqlRes;

	try {
		if (!isInVector((string)req->method_string(), methods)) { throw 1; }
		if (req->version() != 11) { throw 2; }
		if (!(req->find(http::field::host)->value() == "cubery.ru" || req->find(http::field::host)->value() == "api.cubery.ru")) { throw 5; }
		host = (string)req->find(http::field::host)->value();
		cookies = parseCookies((string)req->find(http::field::cookie)->value());
		params = parseTarget((string)req->target(), target);
		method = boost::to_lower_copy((string)req->method_string());
		if (!isLang(cookies["lang"])) { res->set(http::field::set_cookie, "lang=ru; Domain=cubery.ru; Path=/; Max-Age=31536000"); }
		res->body() = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";

		try {
			if (target == "") {
				res->body() = "Main page";
			}
			else if (target == "/signup") {
				sqlReq = "select title, content from content where idKey = 'reg' limit 1";
				sqlRes = stmt->executeQuery(sqlReq);
				sqlRes.res->next();
				res->body() += "<title>" + sqlRes.getString("title") + "</title></head><body>";
				if (isUserLoggedIn(cookies["uuid"], stmt)) {
					res->body() += "Already logged in";
					throw 0;
				}
				res->body() += sqlRes.getString("content") + " <br><br>In this request:<br>";
				stringstream ss;
				ss << boost::beast::buffers(req->body().data());
				res->body() += ss.str() + "<br>";
				if (ss.str().size() == 0) {
					throw 0;
				}
				const vector<string> posValues = { "name", "lname", "pass", "mail", "birth" };
				map<string, string> bodyParams = parseParam(ss.str(), posValues);
				for (auto item : bodyParams) {
					res->body() += item.first + " = " + item.second + "<br><br>";
				}
				unsigned int status = 0;
				if (isEmail(bodyParams["mail"])) {
					sqlReq = "select id from accounts where email = '" + bodyParams["mail"] + "' limit 1";
					sqlRes = stmt->executeQuery(sqlReq);
					if (sqlRes.res->next()) {
						status = 1;
					}
					else {
						status = registerUser(bodyParams, sqlReq);
					}
				}
				else {
					status = 2;
				}
				switch (status) {
				case 0: {stmt->execute(sqlReq); res->body() += "Success"; break; }
				case 1: {res->body() += "Error: already registered"; break; }
				case 2: {res->body() += "Error: invalid email"; break; }
				case 3: {res->body() += "Error: invalid name"; break; }
				case 4: {res->body() += "Error: invalid last name"; break; }
				case 5: {res->body() += "Error: invalid password"; break; }
				case 6: {res->body() += "Error: invalid birth date"; break; }
				}
				res->body() += "</body></html>";
			}
			else if (target == "/login") {
				sqlReq = "select title,content from content where idKey = 'login' limit 1";
				sqlRes = stmt->executeQuery(sqlReq);
				sqlRes.res->next();
				res->body() += "<title>" + sqlRes.getString("title") +
					"</title></head><body>" + sqlRes.getString("content");
				res->body() += "<br><br>In this request:<br>";
				stringstream ss;
				ss << boost::beast::buffers(req->body().data());
				res->body() += ss.str() + "<br>";
				res->body() += "uuid = " + cookies["uuid"] + "<br><br>";
				const vector<string> posValues = { "pass", "mail" };
				map<string, string> bodyParams = parseParam(ss.str(), posValues);
				unsigned int status = 0;
				string uuid = "";
				if (isUserLoggedIn(cookies["uuid"], stmt)) { status = 2; }
				else {
					status = loginUser(bodyParams, stmt, uuid);
				}
				switch (status) {
				case 0: {
					res->set(http::field::set_cookie, "uuid=" + uuid + "; Domain=cubery.ru; Path=/; Max-Age=31536000; Secure");
					res->body() += "Welcome, " + resolveUserByUUID(uuid, stmt);
					break;
				}
				case 1: { res->body() += "Wrong email or password"; break; }
				case 2: { res->body() += "Already logged in     <form method=\"GET\" action=\"/logout\"><input type=\"submit\" value=\"Logout\"></form>"; break; }
				}
				res->body() += "</body></html>";
			}
			else if (target == "/logout") {
				if (isUserLoggedIn(cookies["uuid"], stmt)) {
					sqlReq = "delete from sessions where uuid = '" + cookies["uuid"] + "'";
					stmt->execute(sqlReq);
				}
				res->set(http::field::set_cookie, "uuid=; Domain=cubery.ru; path=/; Max-Age=1; Secure");
				res->body() = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"0; URL=/\"></head></html>";
			}
			else if (target == "/edit") {
				if (!isUserLoggedIn(cookies["uuid"], stmt)) {
					res->body() = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"0; URL=/login\"></head><body>";
					throw 0;
				}
				stringstream ss;
				ss << boost::beast::buffers(req->body().data());
				const vector<string> posValues = { "title", "code", "ref", "category" };
				map<string, string> bodyParams = parseParam(ss.str(), posValues);
				string msg = "";
				unsigned int status = createArticle(bodyParams, stmt, cookies["lang"], msg);
				if (status != 0) {
					sqlReq = "select title, content from content where idKey = 'edit' limit 4";
					sqlRes = stmt->executeQuery(sqlReq);
					sqlRes.res->next();
					res->body() += "<title>" + sqlRes.getString("title") + "</title><body>" +
						sqlRes.getString("content");
					vector<string> content = getContent(sqlRes.res.get());
					res->body() += " value=\"" + htmlGuard(bodyParams["title"]) + "\"";
					res->body() += content[0] + htmlGuard(bodyParams["code"]);
					res->body() += content[1] + "<p><input type=\"text\" name=\"category\" value=\"" + htmlGuard(bodyParams["category"]) + "\"></p>";
					// if edit must not be category
					res->body() += content[2];
					if (params["ref"] != "") {
						res->body() += "<input type=\"hidden\" name=\"ref\" value=\"" + params["ref"] +
							"\" form=\"newArticle\">";
					}
					res->body() += "<br>";
				}
				switch (status) {
				case 0: { res->body() = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta http-equiv=\"refresh\" content=\"0; URL=/" + bodyParams["title"] + "\"></head><body>"; break; }
				case 1: { res->body() += "That article alreay exists"; break; }
				case 2: { res->body() += "Article with that name already exists"; break; }
				case 3: { res->body() += "Unknown language"; break; }
				case 4: { res->body() += msg; break; }
				case 5: { res->body() += "Invalid category"; break; }
				case 6: { res->body() += "Invalid title"; break; }
				}
				throw 0;
			}
			else if (target == "/search") {
				string regexp = "";
				bool live = false;
				if (params["live"] == "1") {
					live = true;
				}
				res->set(http::field::content_type, "application/json; charset=utf-8");
				res->body() = "{\"response\":{\"status\":\"ok\",\"code\":0,\"result\":[";
				params["q"] = prepareSearchString(params["q"], regexp);
				if (params["q"] == "") {}
				if (live || !isNumber(params["offset"])) { params["offset"] = "0"; }
				if (!live) {
					sqlReq = "(select title, substring(content,1,50) as content from articles where match (title, content) against ('" +
						params["q"] + "')) union (select title, substring(content,1,50) as content from articles where title regexp '" +
						regexp + "' or content regexp '" + regexp + "') limit " + params["offset"] + ", 30";
				}
				else {
					sqlReq = "(select title from articles where title like '" + params["q"] + "%') union (select title from articles " +
						"where match (title, content) against ('" + params["q"] + "')) limit " + params["offset"] + ",30";
				}
				sqlRes = stmt->executeQuery(sqlReq);
				bool one = false;
				while (sqlRes.res->next()) {
					if (one) {
						res->body() += ",";
					}
					else {
						one = true;
					}
					res->body() += "{\"title\":\"" + sqlRes.getString("title");
					if (!live) {
						res->body() += "\",\"content\":\"" +
							sqlRes.getString("content");
					}
					res->body() += "\"}";
				}
				res->body() += "]}}";
			}
			else if (s_copy(target, 0, 4) == "/js/") {
				sqlReq = "select content from res where title = '" + s_copy_sql(target, 4, target.size() - 4) + "'";
				sqlRes = stmt->executeQuery(sqlReq);
				if (sqlRes.res->next()) {
					res->body() = sqlRes.getString("content");
				}
				else {
					throw 3;
				}
			}
			else if (s_copy(target, 0, 5) == "/css/") {
				sqlReq = "select content from res where title = '" + s_copy_sql(target, 5, target.size() - 5) + "'";
				sqlRes = stmt->executeQuery(sqlReq);
				if (sqlRes.res->next()) {
					res->body() = sqlRes.getString("content");
				}
				else {
					throw 3;
				}
			}
			else if (s_copy(target, 0, 5) == "/img/") {
				sqlReq = "select content from images where title = '" + s_copy_sql(target, 5, target.size() - 5) + "'";
				sqlRes = stmt->executeQuery(sqlReq);
				if (sqlRes.res->next()) {
					istream* data = sqlRes.res->getBlob("content");
					ostringstream oss;
					oss << data->rdbuf();
					res->set(http::field::content_type, "image/jpg");
					res->body() = oss.str();
				}
				else {
					throw 3;
				}
			}
			else if (target == "/mm") {
				sqlReq = "select content from res where title = 'mm' limit 1";
				sqlRes = stmt->executeQuery(sqlReq);
				sqlRes.res->next();
				res->body() = sqlRes.getString("content");
			}
			else if (target == "/jq") {
				sqlReq = "select content from res where title = 'jquery' limit 1";
				sqlRes = stmt->executeQuery(sqlReq);
				sqlRes.res->next();
				res->body() = sqlRes.getString("content");
				res->set(http::field::content_type, "text/javascript");
			}
			else {
				if (target.size() < 65) {
					string trg = copy(target, 1, target.size() - 1);
					if (getArticleIfExistsByTitle(trg, cookies["lang"], stmt, sqlRes)) {
						res->body() += "<title>" + trg + "</title><body><br>" +
							"<span style=\"font-size: 35px;\">" + trg + "</span><br>" +
							sqlRes.getString("html");
						throw 0;
					}
				}
				throw 3;
			}
		}
		catch (sql::SQLException e) {
			throw 4;
		}

		res->result(http::status::ok);
	}
	catch (int errorID) {
		switch (errorID) {
		case 0: { res->result(http::status::ok); res->body() += "</body></html>"; break; }
		case 1: { res->result(http::status::not_implemented); res->body() = "not implemented"; break; }
		case 2: { res->result(http::status::http_version_not_supported); res->body() = "http version not supported"; break; }
		case 3: { res->result(http::status::not_found); res->body() = "not found"; break; }
		case 4: { res->result(http::status::internal_server_error); res->body() = " internal server error"; break; }
		case 5: { res->result(http::status::bad_request);  res->body() = "bad request"; break; }
		}
	}
	res->prepare_payload();
	return res;
}