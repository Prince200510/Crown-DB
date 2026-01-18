
#include "SQLParser.h"

#include <cctype>

namespace SQLParser {

enum class TokenType {
	Identifier,
	Number,
	String,
	Star,
	Comma,
	LParen,
	RParen,
	Semicolon,
	Dash,
	End,
	CREATE,
	DATABASE,
	DROP,
	TABLE,
	USE,
	INSERT,
	INTO,
	VALUES,
	SELECT,
	FROM,
	TRUNCATE,
	ALTER,
	ADD,
	COLUMN,
	INT,
	VARCHAR,
	DATE
};

struct Token {
	TokenType type;
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
	auto push = [&](TokenType t, std::string v) {
		out.push_back({t, std::move(v)});
	};

	auto keywordToTokenType = [&](const std::string& upperIdent) -> TokenType {
		if (upperIdent == "CREATE") return TokenType::CREATE;
		if (upperIdent == "DATABASE") return TokenType::DATABASE;
		if (upperIdent == "DROP") return TokenType::DROP;
		if (upperIdent == "TABLE") return TokenType::TABLE;
		if (upperIdent == "USE") return TokenType::USE;
		if (upperIdent == "INSERT") return TokenType::INSERT;
		if (upperIdent == "INTO") return TokenType::INTO;
		if (upperIdent == "VALUES") return TokenType::VALUES;
		if (upperIdent == "SELECT") return TokenType::SELECT;
		if (upperIdent == "FROM") return TokenType::FROM;
		if (upperIdent == "TRUNCATE") return TokenType::TRUNCATE;
		if (upperIdent == "ALTER") return TokenType::ALTER;
		if (upperIdent == "ADD") return TokenType::ADD;
		if (upperIdent == "COLUMN") return TokenType::COLUMN;
		if (upperIdent == "INT") return TokenType::INT;
		if (upperIdent == "VARCHAR") return TokenType::VARCHAR;
		if (upperIdent == "DATE") return TokenType::DATE;
		return TokenType::Identifier;
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
			std::string ident = sql.substr(start, i - start);
			push(keywordToTokenType(upper(ident)), std::move(ident));
			continue;
		}

		if (std::isdigit(ch)) {
			size_t start = i;
			++i;
			while (i < sql.size() && std::isdigit(static_cast<unsigned char>(sql[i]))) ++i;
			push(TokenType::Number, sql.substr(start, i - start));
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
			push(TokenType::String, value);
			continue;
		}

		switch (sql[i]) {
			case '*': push(TokenType::Star, "*"); ++i; break;
			case ',': push(TokenType::Comma, ","); ++i; break;
			case '(': push(TokenType::LParen, "("); ++i; break;
			case ')': push(TokenType::RParen, ")"); ++i; break;
			case ';': push(TokenType::Semicolon, ";"); ++i; break;
			case '-': push(TokenType::Dash, "-"); ++i; break;
			default:
				error = "Unexpected character in SQL";
				return {};
		}
	}

	push(TokenType::End, "");
	return out;
}

static bool expect(const std::vector<Token>& toks, size_t& pos, TokenType type) {
	if (toks[pos].type != type) return false;
	++pos;
	return true;
}

static bool parseIdentifier(const std::vector<Token>& toks, size_t& pos, std::string& out) {
	if (toks[pos].type != TokenType::Identifier) return false;
	out = toks[pos].text;
	++pos;
	return true;
}

static bool parseTypeSpec(const std::vector<Token>& toks, size_t& pos, int& outType) {
	constexpr int TYPE_INT = 1;
	constexpr int TYPE_VARCHAR = 2;
	constexpr int TYPE_DATE = 3;

	if (toks[pos].type == TokenType::INT) {
		++pos;
		outType = TYPE_INT;
		return true;
	}
	if (toks[pos].type == TokenType::DATE) {
		++pos;
		outType = TYPE_DATE;
		return true;
	}
	if (toks[pos].type == TokenType::VARCHAR) {
		++pos;
		int len = 255;
		if (toks[pos].type == TokenType::LParen) {
			++pos;
			if (toks[pos].type != TokenType::Number) return false;
			len = std::stoi(toks[pos].text);
			++pos;
			if (!expect(toks, pos, TokenType::RParen)) return false;
		}
		if (len <= 0) return false;
		outType = TYPE_VARCHAR | (len << 16);
		return true;
	}
	return false;
}

static bool parseValue(const std::vector<Token>& toks, size_t& pos, std::string& out) {
	auto isBareWord = [](TokenType t) {
		switch (t) {
			case TokenType::Identifier:
			case TokenType::CREATE:
			case TokenType::DATABASE:
			case TokenType::DROP:
			case TokenType::TABLE:
			case TokenType::USE:
			case TokenType::INSERT:
			case TokenType::INTO:
			case TokenType::VALUES:
			case TokenType::SELECT:
			case TokenType::FROM:
			case TokenType::TRUNCATE:
			case TokenType::ALTER:
			case TokenType::ADD:
			case TokenType::COLUMN:
			case TokenType::INT:
			case TokenType::VARCHAR:
			case TokenType::DATE:
				return true;
			default:
				return false;
		}
	};

	if (toks[pos].type == TokenType::String) {
		out = toks[pos].text;
		++pos;
		return true;
	}

	if (toks[pos].type == TokenType::Number) {
		const std::string a = toks[pos].text;
		size_t p = pos;
		++p;
		if (toks[p].type == TokenType::Dash && toks[p + 1].type == TokenType::Number && toks[p + 2].type == TokenType::Dash && toks[p + 3].type == TokenType::Number) {
			out = a + "-" + toks[p + 1].text + "-" + toks[p + 3].text;
			pos = p + 4;
			return true;
		}
		out = a;
		++pos;
		return true;
	}

	if (isBareWord(toks[pos].type)) {
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
	while (toks[pos].type != TokenType::End && toks[pos].type != TokenType::Semicolon) {
		break;
	}

	if (toks.back().type != TokenType::End) {
		toks.push_back({TokenType::End, ""});
	}

	if (toks.size() < 2 || toks[toks.size() - 2].type != TokenType::Semicolon) {
		r.ok = false;
		r.error = "Statement must end with ';'";
		return r;
	}

	if (toks[pos].type == TokenType::CREATE) {
		++pos;
		if (toks[pos].type == TokenType::DATABASE) {
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
		if (toks[pos].type == TokenType::TABLE) {
			++pos;
			std::string table;
			if (!parseIdentifier(toks, pos, table)) {
				r.ok = false;
				r.error = "Expected table name";
				return r;
			}
			if (!expect(toks, pos, TokenType::LParen)) {
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
				if (toks[pos].type == TokenType::Comma) {
					++pos;
					continue;
				}
				break;
			}
			if (!expect(toks, pos, TokenType::RParen)) {
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

	if (toks[pos].type == TokenType::DROP) {
		++pos;
		if (toks[pos].type == TokenType::DATABASE) {
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
		if (toks[pos].type == TokenType::TABLE) {
			++pos;
			std::string table;
			if (!parseIdentifier(toks, pos, table)) {
				r.ok = false;
				r.error = "Expected table name";
				return r;
			}
			r.statement.type = StatementType::DropTable;
			r.statement.tableName = table;
			r.ok = true;
			return r;
		}
		r.ok = false;
		r.error = "Expected DATABASE or TABLE";
		return r;
	}

	if (toks[pos].type == TokenType::USE) {
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

	if (toks[pos].type == TokenType::TRUNCATE) {
		++pos;
		if (toks[pos].type != TokenType::TABLE) {
			r.ok = false;
			r.error = "Expected TABLE";
			return r;
		}
		++pos;
		std::string table;
		if (!parseIdentifier(toks, pos, table)) {
			r.ok = false;
			r.error = "Expected table name";
			return r;
		}
		r.statement.type = StatementType::TruncateTable;
		r.statement.tableName = table;
		r.ok = true;
		return r;
	}

	if (toks[pos].type == TokenType::ALTER) {
		++pos;
		if (toks[pos].type != TokenType::TABLE) {
			r.ok = false;
			r.error = "Expected TABLE";
			return r;
		}
		++pos;
		std::string table;
		if (!parseIdentifier(toks, pos, table)) {
			r.ok = false;
			r.error = "Expected table name";
			return r;
		}

		if (toks[pos].type == TokenType::ADD) {
			++pos;
			if (toks[pos].type == TokenType::COLUMN) {
				++pos;
			}
			std::string col;
			if (!parseIdentifier(toks, pos, col)) {
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
			r.statement.type = StatementType::AlterAddColumn;
			r.statement.tableName = table;
			r.statement.columnName = col;
			r.statement.columnType = type;
			r.ok = true;
			return r;
		}

		if (toks[pos].type == TokenType::DROP) {
			++pos;
			if (toks[pos].type == TokenType::COLUMN) {
				++pos;
			}
			std::string col;
			if (!parseIdentifier(toks, pos, col)) {
				r.ok = false;
				r.error = "Expected column name";
				return r;
			}
			r.statement.type = StatementType::AlterDropColumn;
			r.statement.tableName = table;
			r.statement.columnName = col;
			r.ok = true;
			return r;
		}

		r.ok = false;
		r.error = "Expected ADD or DROP";
		return r;
	}

	if (toks[pos].type == TokenType::INSERT) {
		++pos;
		if (toks[pos].type != TokenType::INTO) {
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
		if (toks[pos].type != TokenType::VALUES) {
			r.ok = false;
			r.error = "Expected VALUES";
			return r;
		}
		++pos;
		if (!expect(toks, pos, TokenType::LParen)) {
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
			if (toks[pos].type == TokenType::Comma) {
				++pos;
				continue;
			}
			break;
		}
		if (!expect(toks, pos, TokenType::RParen)) {
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

	if (toks[pos].type == TokenType::SELECT) {
		++pos;
		if (!expect(toks, pos, TokenType::Star)) {
			r.ok = false;
			r.error = "Only SELECT * supported";
			return r;
		}
		if (toks[pos].type != TokenType::FROM) {
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
