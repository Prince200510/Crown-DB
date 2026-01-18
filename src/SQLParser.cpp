
#include "SQLParser.h"

#include <cctype>

namespace SQLParser {

enum class TokType {
	Identifier,
	Number,
	String,
	Star,
	Comma,
	LParen,
	RParen,
	Semicolon,
	Dash,
	End
};

struct Token {
	TokType type;
	std::string text;
};

static std::string upper(std::string s) {
	for (char& ch : s) {
		ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
	}
	return s;
}

static std::string trimCopy(std::string s) {
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
	return s;
}

static std::vector<Token> tokenize(const std::string& sql, std::string& error) {
	std::vector<Token> out;
	size_t i = 0;
	auto push = [&](TokType t, std::string v) {
		out.push_back({t, std::move(v)});
	};

	while (i < sql.size()) {
		unsigned char ch = static_cast<unsigned char>(sql[i]);
		if (std::isspace(ch)) {
			++i;
			continue;
		}

		if (std::isalpha(ch) || ch == '_') {
			size_t start = i;
			++i;
			while (i < sql.size()) {
				unsigned char c = static_cast<unsigned char>(sql[i]);
				if (!(std::isalnum(c) || c == '_')) break;
				++i;
			}
			push(TokType::Identifier, sql.substr(start, i - start));
			continue;
		}

		if (std::isdigit(ch)) {
			size_t start = i;
			++i;
			while (i < sql.size() && std::isdigit(static_cast<unsigned char>(sql[i]))) ++i;
			push(TokType::Number, sql.substr(start, i - start));
			continue;
		}

		if (sql[i] == '\'' || sql[i] == '"') {
			const char quote = sql[i];
			++i;
			std::string value;
			while (i < sql.size()) {
				char c = sql[i];
				if (c == quote) {
					++i;
					break;
				}
				value.push_back(c);
				++i;
			}
			push(TokType::String, value);
			continue;
		}

		switch (sql[i]) {
			case '*': push(TokType::Star, "*"); ++i; break;
			case ',': push(TokType::Comma, ","); ++i; break;
			case '(': push(TokType::LParen, "("); ++i; break;
			case ')': push(TokType::RParen, ")"); ++i; break;
			case ';': push(TokType::Semicolon, ";"); ++i; break;
			case '-': push(TokType::Dash, "-"); ++i; break;
			default:
				error = "Unexpected character in SQL";
				return {};
		}
	}

	push(TokType::End, "");
	return out;
}

static bool matchKeyword(const Token& t, const char* kw) {
	if (t.type != TokType::Identifier) return false;
	return upper(t.text) == kw;
}

static bool expect(const std::vector<Token>& toks, size_t& pos, TokType type) {
	if (toks[pos].type != type) return false;
	++pos;
	return true;
}

static bool parseIdentifier(const std::vector<Token>& toks, size_t& pos, std::string& out) {
	if (toks[pos].type != TokType::Identifier) return false;
	out = toks[pos].text;
	++pos;
	return true;
}

static bool parseTypeSpec(const std::vector<Token>& toks, size_t& pos, int& outType) {
	if (toks[pos].type != TokType::Identifier) return false;
	const std::string t = upper(toks[pos].text);
	++pos;

	constexpr int TYPE_INT = 1;
	constexpr int TYPE_VARCHAR = 2;
	constexpr int TYPE_DATE = 3;

	if (t == "INT") {
		outType = TYPE_INT;
		return true;
	}
	if (t == "DATE") {
		outType = TYPE_DATE;
		return true;
	}
	if (t == "VARCHAR") {
		int len = 255;
		if (toks[pos].type == TokType::LParen) {
			++pos;
			if (toks[pos].type != TokType::Number) return false;
			len = std::stoi(toks[pos].text);
			++pos;
			if (!expect(toks, pos, TokType::RParen)) return false;
		}
		if (len <= 0) return false;
		outType = TYPE_VARCHAR | (len << 16);
		return true;
	}
	return false;
}

static bool parseValue(const std::vector<Token>& toks, size_t& pos, std::string& out) {
	if (toks[pos].type == TokType::String) {
		out = toks[pos].text;
		++pos;
		return true;
	}

	if (toks[pos].type == TokType::Number) {
		const std::string a = toks[pos].text;
		size_t p = pos;
		++p;
		if (toks[p].type == TokType::Dash && toks[p + 1].type == TokType::Number && toks[p + 2].type == TokType::Dash && toks[p + 3].type == TokType::Number) {
			out = a + "-" + toks[p + 1].text + "-" + toks[p + 3].text;
			pos = p + 4;
			return true;
		}
		out = a;
		++pos;
		return true;
	}

	if (toks[pos].type == TokType::Identifier) {
		out = toks[pos].text;
		++pos;
		return true;
	}

	return false;
}

ParseResult parse(const std::string& sql) {
	ParseResult r;
	std::string error;
	auto toks = tokenize(sql, error);
	if (toks.empty()) {
		r.ok = false;
		r.error = error.empty() ? "Failed to tokenize SQL" : error;
		return r;
	}

	size_t pos = 0;
	while (toks[pos].type != TokType::End && toks[pos].type != TokType::Semicolon) {
		break;
	}

	if (toks.back().type != TokType::End) {
		toks.push_back({TokType::End, ""});
	}

	if (toks.size() < 2 || toks[toks.size() - 2].type != TokType::Semicolon) {
		r.ok = false;
		r.error = "Statement must end with ';'";
		return r;
	}

	if (matchKeyword(toks[pos], "CREATE")) {
		++pos;
		if (matchKeyword(toks[pos], "DATABASE")) {
			++pos;
			std::string db;
			if (!parseIdentifier(toks, pos, db)) {
				r.ok = false;
				r.error = "Expected database name";
				return r;
			}
			r.statement.type = StatementType::CreateDatabase;
			r.statement.databaseName = db;
			r.ok = true;
			return r;
		}
		if (matchKeyword(toks[pos], "TABLE")) {
			++pos;
			std::string table;
			if (!parseIdentifier(toks, pos, table)) {
				r.ok = false;
				r.error = "Expected table name";
				return r;
			}
			if (!expect(toks, pos, TokType::LParen)) {
				r.ok = false;
				r.error = "Expected '('";
				return r;
			}
			std::vector<ColumnDef> cols;
			while (true) {
				std::string colName;
				if (!parseIdentifier(toks, pos, colName)) {
					r.ok = false;
					r.error = "Expected column name";
					return r;
				}
				int type = 0;
				if (!parseTypeSpec(toks, pos, type)) {
					r.ok = false;
					r.error = "Invalid column type";
					return r;
				}
				cols.push_back({colName, type});
				if (toks[pos].type == TokType::Comma) {
					++pos;
					continue;
				}
				break;
			}
			if (!expect(toks, pos, TokType::RParen)) {
				r.ok = false;
				r.error = "Expected ')'";
				return r;
			}
			r.statement.type = StatementType::CreateTable;
			r.statement.tableName = table;
			r.statement.columns = std::move(cols);
			r.ok = true;
			return r;
		}
	}

	if (matchKeyword(toks[pos], "DROP")) {
		++pos;
		if (!matchKeyword(toks[pos], "DATABASE")) {
			r.ok = false;
			r.error = "Expected DATABASE";
			return r;
		}
		++pos;
		std::string db;
		if (!parseIdentifier(toks, pos, db)) {
			r.ok = false;
			r.error = "Expected database name";
			return r;
		}
		r.statement.type = StatementType::DropDatabase;
		r.statement.databaseName = db;
		r.ok = true;
		return r;
	}

	if (matchKeyword(toks[pos], "USE")) {
		++pos;
		std::string db;
		if (!parseIdentifier(toks, pos, db)) {
			r.ok = false;
			r.error = "Expected database name";
			return r;
		}
		r.statement.type = StatementType::UseDatabase;
		r.statement.databaseName = db;
		r.ok = true;
		return r;
	}

	if (matchKeyword(toks[pos], "INSERT")) {
		++pos;
		if (!matchKeyword(toks[pos], "INTO")) {
			r.ok = false;
			r.error = "Expected INTO";
			return r;
		}
		++pos;
		std::string table;
		if (!parseIdentifier(toks, pos, table)) {
			r.ok = false;
			r.error = "Expected table name";
			return r;
		}
		if (!matchKeyword(toks[pos], "VALUES")) {
			r.ok = false;
			r.error = "Expected VALUES";
			return r;
		}
		++pos;
		if (!expect(toks, pos, TokType::LParen)) {
			r.ok = false;
			r.error = "Expected '('";
			return r;
		}
		std::vector<std::string> values;
		while (true) {
			std::string v;
			if (!parseValue(toks, pos, v)) {
				r.ok = false;
				r.error = "Invalid value";
				return r;
			}
			values.push_back(v);
			if (toks[pos].type == TokType::Comma) {
				++pos;
				continue;
			}
			break;
		}
		if (!expect(toks, pos, TokType::RParen)) {
			r.ok = false;
			r.error = "Expected ')'";
			return r;
		}
		r.statement.type = StatementType::InsertInto;
		r.statement.tableName = table;
		r.statement.values = std::move(values);
		r.ok = true;
		return r;
	}

	if (matchKeyword(toks[pos], "SELECT")) {
		++pos;
		if (!expect(toks, pos, TokType::Star)) {
			r.ok = false;
			r.error = "Only SELECT * supported";
			return r;
		}
		if (!matchKeyword(toks[pos], "FROM")) {
			r.ok = false;
			r.error = "Expected FROM";
			return r;
		}
		++pos;
		std::string table;
		if (!parseIdentifier(toks, pos, table)) {
			r.ok = false;
			r.error = "Expected table name";
			return r;
		}
		r.statement.type = StatementType::SelectAll;
		r.statement.tableName = table;
		r.ok = true;
		return r;
	}

	r.ok = false;
	r.error = "Unsupported SQL statement";
	return r;
}

}
