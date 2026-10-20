
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
	DATE,
	WHERE,
	COUNT,
	SUM,
	AVG,
	MIN,
	MAX,
	FLOAT,
	DOUBLE,
	BOOL,
	BOOLEAN,
	TEXT,
	PRIMARY,
	KEY,
	NOT,
	NULL_VAL,
	UNIQUE,
	AND,
	OR,
	DOT,
	EQ,
	NE,
	LT,
	LE,
	GT,
	GE
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
		if(upperIdent == "CREATE") return TokenType::CREATE;
		if(upperIdent == "DATABASE") return TokenType::DATABASE;
		if(upperIdent == "DROP") return TokenType::DROP;
		if(upperIdent == "TABLE") return TokenType::TABLE;
		if(upperIdent == "USE") return TokenType::USE;
		if(upperIdent == "INSERT") return TokenType::INSERT;
		if(upperIdent == "FLOAT") return TokenType::FLOAT;
		if(upperIdent == "DOUBLE") return TokenType::DOUBLE;
		if(upperIdent == "REAL") return TokenType::FLOAT;
		if(upperIdent == "BOOL") return TokenType::BOOL;
		if(upperIdent == "BOOLEAN") return TokenType::BOOL;
		if(upperIdent == "TEXT") return TokenType::TEXT;
		if(upperIdent == "INTEGER") return TokenType::INT;
		if(upperIdent == "PRIMARY") return TokenType::PRIMARY;
		if(upperIdent == "KEY") return TokenType::KEY;
		if(upperIdent == "NOT") return TokenType::NOT;
		if(upperIdent == "NULL") return TokenType::NULL_VAL;
		if(upperIdent == "UNIQUE") return TokenType::UNIQUE;
		if(upperIdent == "INTO") return TokenType::INTO;
		if(upperIdent == "VALUES") return TokenType::VALUES;
		if(upperIdent == "SELECT") return TokenType::SELECT;
		if(upperIdent == "FROM") return TokenType::FROM;
		if(upperIdent == "TRUNCATE") return TokenType::TRUNCATE;
		if(upperIdent == "ALTER") return TokenType::ALTER;
		if(upperIdent == "ADD") return TokenType::ADD;
		if(upperIdent == "COLUMN") return TokenType::COLUMN;
		if(upperIdent == "INT") return TokenType::INT;
		if(upperIdent == "VARCHAR") return TokenType::VARCHAR;
		if(upperIdent == "DATE") return TokenType::DATE;
		if(upperIdent == "WHERE") return TokenType::WHERE;
		if(upperIdent == "COUNT") return TokenType::COUNT;
		if(upperIdent == "SUM") return TokenType::SUM;
		if(upperIdent == "AVG") return TokenType::AVG;
		if(upperIdent == "MIN") return TokenType::MIN;
		if(upperIdent == "MAX") return TokenType::MAX;
		if(upperIdent == "AND") return TokenType::AND;
		if(upperIdent == "OR") return TokenType::OR;
		return TokenType::Identifier;
	};

	while (i < sql.size()) {
		unsigned char ch = static_cast<unsigned char>(sql[i]);
		if (std::isspace(ch)) {
			i++;
			continue;
		}

		if (std::isalpha(ch) || ch == '_') {
			size_t start = i;
			i++;
			while (i < sql.size()) {
				unsigned char c = static_cast<unsigned char>(sql[i]);
				if (!(std::isalnum(c) || c == '_')) break;
				i++;
			}
			std::string ident = sql.substr(start, i - start);
			const TokenType kw = keywordToTokenType(upper(ident));
			push(kw, std::move(ident));
			continue;
		}

				if (std::isdigit(ch)) {
			size_t start = i;
			i++;
			while (i < sql.size() && std::isdigit(static_cast<unsigned char>(sql[i]))) i++;
			if (i < sql.size() && sql[i] == '.' && i + 1 < sql.size() && std::isdigit(static_cast<unsigned char>(sql[i + 1]))) {
				i++;
				while (i < sql.size() && std::isdigit(static_cast<unsigned char>(sql[i]))) i++;
			}
			push(TokenType::Number, sql.substr(start, i - start));
			continue;
		}

		if (ch == '.' && i + 1 < sql.size() && std::isdigit(static_cast<unsigned char>(sql[i + 1]))) {
			size_t start = i;
			i += 2;
			while (i < sql.size() && std::isdigit(static_cast<unsigned char>(sql[i]))) i++;
			push(TokenType::Number, sql.substr(start, i - start));
			continue;
		}


		if (sql[i] == '\'' || sql[i] == '"') {
			const char quote = sql[i];
			i++;
			std::string value;
			while (i < sql.size()) {
				char c = sql[i];
				if (c == quote) {
					i++;
					break;
				}
				value.push_back(c);
				i++;
			}
			push(TokenType::String, value);
			continue;
		}

		switch (sql[i]) {
			case '*': push(TokenType::Star, "*"); i++; break;
			case ',': push(TokenType::Comma, ","); i++; break;
			case '(': push(TokenType::LParen, "("); i++; break;
			case ')': push(TokenType::RParen, ")"); i++; break;
			case ';': push(TokenType::Semicolon, ";"); i++; break;
			case '-': push(TokenType::Dash, "-"); i++; break;
			case '.': push(TokenType::DOT, "."); i++; break;
			case '=':
				if (i + 1 < sql.size() && sql[i + 1] == '=') {
					push(TokenType::EQ, "=");
					i += 2;
				} else {
					push(TokenType::EQ, "=");
					i++;
				}
				break;
			case '>':
				if (i + 1 < sql.size() && sql[i + 1] == '=') { 
					push(TokenType::GE, ">="); i += 2; 
				} else { 
					push(TokenType::GT, ">"); i++; 
				}
				break;
			case '<':
				if (i + 1 < sql.size() && sql[i + 1] == '=') { 
					push(TokenType::LE, "<="); 
					i += 2; 
				} else { 
					push(TokenType::LT, "<"); i++; 
				}
				break;
			case '!':
				if (i + 1 < sql.size() && sql[i + 1] == '=') { 
					push(TokenType::NE, "!="); 
					i += 2; 
					break; 
				}
				error = "Unexpected character in SQL";
				return {};
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
	constexpr int TYPE_FLOAT = 4;
	constexpr int TYPE_BOOL = 5;
	constexpr int TYPE_TEXT = 6;

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
	if (toks[pos].type == TokenType::FLOAT || toks[pos].type == TokenType::DOUBLE) {
		++pos;
		outType = TYPE_FLOAT;
		return true;
	}
	if (toks[pos].type == TokenType::BOOL || toks[pos].type == TokenType::BOOLEAN) {
		++pos;
		outType = TYPE_BOOL;
		return true;
	}
	if (toks[pos].type == TokenType::TEXT) {
		++pos;
		outType = TYPE_TEXT;
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

static int getBinaryPrecedence(TokenType type) {
	switch (type) {
		case TokenType::OR: return 1;
		case TokenType::AND: return 2;
		case TokenType::EQ:
		case TokenType::NE:
		case TokenType::LT:
		case TokenType::LE:
		case TokenType::GT:
		case TokenType::GE:
			return 4;
		case TokenType::Star:
			return 6;
		default:
			return 0;
	}
}

static OpType tokenToOpType(TokenType type) {
	switch (type) {
		case TokenType::OR: return OpType::Or;
		case TokenType::AND: return OpType::And;
		case TokenType::EQ: return OpType::Equal;
		case TokenType::NE: return OpType::NotEqual;
		case TokenType::LT: return OpType::LessThan;
		case TokenType::LE: return OpType::LessEqual;
		case TokenType::GT: return OpType::GreaterThan;
		case TokenType::GE: return OpType::GreaterEqual;
		default: return OpType::Equal;
	}
}

static std::shared_ptr<Expression> parseExpressionPratt(const std::vector<Token>& toks, size_t& pos, int min_prec = 0);

static std::shared_ptr<Expression> parsePrimaryExpression(const std::vector<Token>& toks, size_t& pos) {
	if (pos >= toks.size()) return nullptr;

	if (toks[pos].type == TokenType::LParen) {
		++pos;
		if (pos < toks.size() && toks[pos].type == TokenType::SELECT) {
			std::string sub_sql;
			int depth = 1;
			while (pos < toks.size()) {
				if (toks[pos].type == TokenType::LParen) {
					depth++;
					sub_sql += "(";
					pos++;
				} else if (toks[pos].type == TokenType::RParen) {
					depth--;
					pos++;
					if (depth == 0) break;
					sub_sql += ")";
				} else {
					if (!sub_sql.empty() && sub_sql.back() != '(') {
						sub_sql += " ";
					}
					sub_sql += toks[pos].text;
					pos++;
				}
			}
			return std::make_shared<SubqueryExpression>(std::move(sub_sql));
		}

		auto expr = parseExpressionPratt(toks, pos, 0);
		if (!expect(toks, pos, TokenType::RParen)) return nullptr;
		return expr;
	}

	if (toks[pos].type == TokenType::NOT) {
		++pos;
		auto sub = parsePrimaryExpression(toks, pos);
		if (!sub) return nullptr;
		return std::make_shared<UnaryExpression>(OpType::Not, std::move(sub));
	}

	if (toks[pos].type == TokenType::Number) {
		std::string s = toks[pos].text;
		++pos;
		if (s.find('.') != std::string::npos) {
			return std::make_shared<ConstantExpression>(TypedValue(std::stod(s)));
		}
		return std::make_shared<ConstantExpression>(TypedValue(static_cast<int64_t>(std::stoll(s))));
	}

	if (toks[pos].type == TokenType::String) {
		std::string s = toks[pos].text;
		++pos;
		return std::make_shared<ConstantExpression>(TypedValue(std::move(s)));
	}

	if (toks[pos].type == TokenType::NULL_VAL) {
		++pos;
		return std::make_shared<ConstantExpression>(TypedValue::make_null());
	}

	if (toks[pos].type == TokenType::Identifier || toks[pos].type == TokenType::BOOL) {
		std::string name = toks[pos].text;
		++pos;
		std::string upper_name = upper(name);
		if (upper_name == "TRUE") {
			return std::make_shared<ConstantExpression>(TypedValue(true));
		}
		if (upper_name == "FALSE") {
			return std::make_shared<ConstantExpression>(TypedValue(false));
		}
		return std::make_shared<ColumnRefExpression>(std::move(name));
	}

	return nullptr;
}

static std::shared_ptr<Expression> parseExpressionPratt(const std::vector<Token>& toks, size_t& pos, int min_prec) {
	auto left = parsePrimaryExpression(toks, pos);
	if (!left) return nullptr;

	while (pos < toks.size()) {
		int prec = getBinaryPrecedence(toks[pos].type);
		if (prec <= min_prec) break;

		TokenType opTok = toks[pos].type;
		OpType op = tokenToOpType(opTok);
		++pos;

		auto right = parseExpressionPratt(toks, pos, prec);
		if (!right) return nullptr;

		left = std::make_shared<BinaryExpression>(op, std::move(left), std::move(right));
	}

	return left;
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
			case TokenType::WHERE:
			case TokenType::COUNT:
			case TokenType::SUM:
			case TokenType::AVG:
			case TokenType::MIN:
			case TokenType::MAX:
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

				ColumnDef col;
				col.name = std::move(colName);
				col.type = type;

				while (pos < toks.size()) {
					if (toks[pos].type == TokenType::PRIMARY) {
						++pos;
						if (toks[pos].type == TokenType::KEY) {
							++pos;
						}
						col.is_primary_key = true;
						col.is_not_null = true;
						col.is_unique = true;
						continue;
					}
					if (toks[pos].type == TokenType::NOT) {
						++pos;
						if (toks[pos].type == TokenType::NULL_VAL) {
							++pos;
						}
						col.is_not_null = true;
						continue;
					}
					if (toks[pos].type == TokenType::UNIQUE) {
						++pos;
						col.is_unique = true;
						continue;
					}
					break;
				}

				if (col.is_primary_key) {
					col.type |= (1 << 8);
				}
				
				if (col.is_not_null) {
					col.type |= (1 << 9);
				}

				if (col.is_unique) {
					col.type |= (1 << 10);
				}

				cols.push_back(std::move(col));
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
		if (toks[pos].type == TokenType::Star) {
			++pos;
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
									if (toks[pos].type == TokenType::WHERE) {
				++pos;
				auto expr = parseExpressionPratt(toks, pos, 0);
				if (!expr) {
					r.ok = false;
					r.error = "Invalid WHERE clause";
					return r;
				}
				r.statement.whereExpr = expr;

				if (auto bin = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
					if (auto col = std::dynamic_pointer_cast<ColumnRefExpression>(bin->left())) {
						if (auto c = std::dynamic_pointer_cast<ConstantExpression>(bin->right())) {
							r.statement.whereColumn = col->col_name();
							switch (bin->op()) {
								case OpType::Equal: r.statement.whereOp = "="; break;
								case OpType::NotEqual: r.statement.whereOp = "!="; break;
								case OpType::LessThan: r.statement.whereOp = "<"; break;
								case OpType::LessEqual: r.statement.whereOp = "<="; break;
								case OpType::GreaterThan: r.statement.whereOp = ">"; break;
								case OpType::GreaterEqual: r.statement.whereOp = ">="; break;
								default: break;
							}
							r.statement.whereValue = c->evaluate({}, {}).to_string();
						}
					}
				}
				r.statement.type = StatementType::SelectWhere;
			} else {
				r.statement.type = StatementType::SelectAll;
			}

			r.statement.tableName = table;
			r.ok = true;
			return r;
		}

		std::vector<std::string> selectCols;
		bool hasAgg = false;
		SQLParser::AggregateFunc aggFunc = SQLParser::AggregateFunc::Count;
		std::string aggCol;
		while (true) {
			if (toks[pos].type == TokenType::Identifier) {
				std::string c;
				if (!parseIdentifier(toks, pos, c)) {
					r.ok = false;
					r.error = "Expected column name";
					return r;
				}
				selectCols.push_back(std::move(c));
			} else {
				SQLParser::AggregateFunc f;
				switch (toks[pos].type) {
					case TokenType::COUNT: f = SQLParser::AggregateFunc::Count; break;
					case TokenType::SUM: f = SQLParser::AggregateFunc::Sum; break;
					case TokenType::AVG: f = SQLParser::AggregateFunc::Avg; break;
					case TokenType::MIN: f = SQLParser::AggregateFunc::Min; break;
					case TokenType::MAX: f = SQLParser::AggregateFunc::Max; break;
					default:
						r.ok = false;
						r.error = "Expected column name, or COUNT/SUM/AVG/MIN/MAX";
						return r;
				}
				if (hasAgg) {
					r.ok = false;
					r.error = "Only one aggregate function is supported";
					return r;
				}
				hasAgg = true;
				aggFunc = f;
				++pos;
				if (!expect(toks, pos, TokenType::LParen)) {
					r.ok = false;
					r.error = "Expected '('";
					return r;
				}
				if (toks[pos].type == TokenType::Star) {
					aggCol = "*";
					++pos;
				} else {
					if (!parseIdentifier(toks, pos, aggCol)) {
						r.ok = false;
						r.error = "Expected column name or '*'";
						return r;
					}
				}
				if (!expect(toks, pos, TokenType::RParen)) {
					r.ok = false;
					r.error = "Expected ')'";
					return r;
				}
			}

			if (toks[pos].type == TokenType::Comma) {
				++pos;
				continue;
			}
			break;
		}
		if (selectCols.empty() && !hasAgg) {
			r.ok = false;
			r.error = "Expected select list";
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
		
		if (toks[pos].type == TokenType::WHERE) {
			++pos;
			auto expr = parseExpressionPratt(toks, pos, 0);
			if (!expr) {
				r.ok = false;
				r.error = "Invalid WHERE clause";
				return r;
			}
			r.statement.whereExpr = expr;

			if (auto bin = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
				if (auto col = std::dynamic_pointer_cast<ColumnRefExpression>(bin->left())) {
					if (auto c = std::dynamic_pointer_cast<ConstantExpression>(bin->right())) {
						r.statement.whereColumn = col->col_name();
						switch (bin->op()) {
							case OpType::Equal: r.statement.whereOp = "="; break;
							case OpType::NotEqual: r.statement.whereOp = "!="; break;
							case OpType::LessThan: r.statement.whereOp = "<"; break;
							case OpType::LessEqual: r.statement.whereOp = "<="; break;
							case OpType::GreaterThan: r.statement.whereOp = ">"; break;
							case OpType::GreaterEqual: r.statement.whereOp = ">="; break;
							default: break;
						}
						r.statement.whereValue = c->evaluate({}, {}).to_string();
					}
				}
			}
		}

		r.statement.tableName = table;
		if (hasAgg) {
			r.statement.aggregateFunc = aggFunc;
			r.statement.columnName = aggCol;
			if (selectCols.empty()) {
				r.statement.type = StatementType::SelectAggregate;
			} else {
				r.statement.type = StatementType::SelectGroupAggregate;
				r.statement.groupColumns = std::move(selectCols);
			}
			
			r.ok = true;
			return r;
		}

		r.statement.type = StatementType::SelectColumns;
		r.statement.selectColumns = std::move(selectCols);
		r.ok = true;
		return r;
	}

	r.ok = false;
	r.error = "Unsupported SQL statement";
	return r;
}

}
