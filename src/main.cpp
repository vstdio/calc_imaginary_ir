#include <memory>
#include <string>
#include <vector>
#include <cctype>
#include <cassert>
#include <iostream>
#include <optional>

class VariableRefAST;
class NumberAST;
class BinaryExpressionAST;

class INodeVisitor
{
public:
	virtual ~INodeVisitor() = default;
	virtual void Visit(const VariableRefAST& variableRef) = 0;
	virtual void Visit(const NumberAST& number) = 0;
	virtual void Visit(const BinaryExpressionAST& binaryExpr) = 0;
};

class ExpressionAST
{
public:
	virtual ~ExpressionAST() = default;
	virtual void Accept(INodeVisitor& visitor)const = 0;
};

class VariableRefAST : public ExpressionAST
{
public:
	explicit VariableRefAST(std::string name)
		: m_name(std::move(name))
	{
	}

	const std::string& GetName()const
	{
		return m_name;
	}

	void Accept(INodeVisitor& visitor)const override
	{
		visitor.Visit(*this);
	}

private:
	std::string m_name;
};

class NumberAST : public ExpressionAST
{
public:
	explicit NumberAST(double value)
		: m_value(value)
	{
	}

	double GetValue()const
	{
		return m_value;
	}

	void Accept(INodeVisitor& visitor)const override
	{
		visitor.Visit(*this);
	}

private:
	double m_value;
};

class BinaryExpressionAST : public ExpressionAST
{
public:
	explicit BinaryExpressionAST(
		std::unique_ptr<ExpressionAST>&& left,
		std::unique_ptr<ExpressionAST>&& right,
		char op)
		: m_left(std::move(left))
		, m_right(std::move(right))
		, m_op(op)
	{
	}

	const ExpressionAST& GetLeft()const
	{
		return *m_left;
	}

	const ExpressionAST& GetRight()const
	{
		return *m_right;
	}

	char GetOperator()const
	{
		return m_op;
	}

	void Accept(INodeVisitor& visitor)const override
	{
		visitor.Visit(*this);
	}

private:
	std::unique_ptr<ExpressionAST> m_left;
	std::unique_ptr<ExpressionAST> m_right;
	char m_op;
};

class SimpleCodeGenerator : public INodeVisitor
{
public:
	std::string Generate(const ExpressionAST& root)
	{
		root.Accept(*this);
		m_code += "%result = " + m_registers.back();
		return m_code;
	}

	void Visit(const VariableRefAST& variableRef) override
	{
		m_registers.push_back("%x" + std::to_string(m_registerId++));
		m_code += m_registers.back() + " = " + "%" + variableRef.GetName() + "\n";
	}

	void Visit(const NumberAST& number) override
	{
		m_registers.push_back("%x" + std::to_string(m_registerId++));
		m_code += m_registers.back() + " = " + std::to_string(number.GetValue()) + "\n";
	}

	void Visit(const BinaryExpressionAST& binaryExpr) override
	{
		binaryExpr.GetLeft().Accept(*this);
		binaryExpr.GetRight().Accept(*this);

		auto right = m_registers.back(); m_registers.pop_back();
		auto left = m_registers.back(); m_registers.pop_back();

		switch (binaryExpr.GetOperator())
		{
		case '+':
			m_registers.push_back("%addtmp" + std::to_string(m_registerId++));
			m_code += m_registers.back() + " = add " + left + " " + right + "\n";
			break;
		case '-':
			m_registers.push_back("%subtmp" + std::to_string(m_registerId++));
			m_code += m_registers.back() + " = sub " + left + " " + right + "\n";
			break;
		case '*':
			m_registers.push_back("%multmp" + std::to_string(m_registerId++));
			m_code += m_registers.back() + " = mul " + left + " " + right + "\n";
			break;
		case '/':
			m_registers.push_back("%divtmp" + std::to_string(m_registerId++));
			m_code += m_registers.back() + " = div " + left + " " + right + "\n";
			break;
		}
	}

private:
	std::string m_code;
	std::vector<std::string> m_registers;
	int m_registerId = 1;
};

struct Token
{
	enum Type
	{
		Number,
		Identifier,
		Plus,
		Minus,
		Mul,
		Div,
		LeftParen,
		RightParen,
		EndOfFile
	};

	Type type;
	std::optional<std::string> value;
};

class Lexer
{
public:
	explicit Lexer(const std::string& text)
		: m_text(text)
		, m_pos(0)
	{
	}

	Token GetNextToken()
	{
		while (m_pos < m_text.length())
		{
			char ch = m_text[m_pos];
			if (std::isspace(ch))
			{
				SkipWhitespaces();
				continue;
			}
			if (std::isdigit(ch))
			{
				return ReadAsNumberConstant();
			}
			if (std::isalpha(ch))
			{
				return ReadAsIdentifier();
			}
			if (ch == '+')
			{
				++m_pos;
				return { Token::Plus };
			}
			if (ch == '-')
			{
				++m_pos;
				return { Token::Minus };
			}
			if (ch == '*')
			{
				++m_pos;
				return { Token::Mul };
			}
			if (ch == '/')
			{
				++m_pos;
				return { Token::Div };
			}
			if (ch == '(')
			{
				++m_pos;
				return { Token::LeftParen };
			}
			if (ch == ')')
			{
				++m_pos;
				return { Token::RightParen };
			}
			throw std::invalid_argument("can't parse character at pos " + std::to_string(m_pos) + ": '" + m_text[m_pos] + "'");
		}
		return Token{ Token::EndOfFile };
	}

private:
	Token ReadAsNumberConstant()
	{
		assert(m_pos < m_text.length());
		assert(std::isdigit(m_text[m_pos]));

		std::string chars;
		while (m_pos < m_text.length() && std::isdigit(m_text[m_pos]))
		{
			chars += m_text[m_pos++];
		}

		if (m_pos < m_text.length() && m_text[m_pos] == '.')
		{
			chars += m_text[m_pos++];
			while (m_pos < m_text.length() && std::isdigit(m_text[m_pos]))
			{
				chars += m_text[m_pos++];
			}
		}

		return { Token::Number, std::move(chars) };
	}

	Token ReadAsIdentifier()
	{
		assert(m_pos < m_text.length());
		assert(std::isalpha(m_text[m_pos]) || m_text[m_pos] == '_');

		std::string chars;
		while (m_pos < m_text.length() && (std::isalnum(m_text[m_pos]) || m_text[m_pos] == '_'))
		{
			chars += m_text[m_pos++];
		}

		return { Token::Identifier, std::move(chars) };
	}

	void SkipWhitespaces()
	{
		while (m_pos < m_text.length() && std::isspace(m_text[m_pos]))
		{
			++m_pos;
		}
	}

private:
	std::string m_text;
	size_t m_pos;
};

class Parser
{
public:
	Parser(std::unique_ptr<Lexer> && lexer)
		: m_lexer(std::move(lexer))
		, m_token(m_lexer->GetNextToken())
	{
	}

	std::unique_ptr<ExpressionAST> ParseAtom()
	{
		if (m_token.type == Token::Number)
		{
			const std::string value = *m_token.value;
			Eat(Token::Number);
			return std::make_unique<NumberAST>(std::stoi(value));
		}
		else if (m_token.type == Token::Identifier)
		{
			auto identifier = *m_token.value;
			Eat(Token::Identifier);
			return std::make_unique<VariableRefAST>(identifier);
		}
		else if (m_token.type == Token::LeftParen)
		{
			Eat(Token::LeftParen);
			auto node = ParseAddSub();
			Eat(Token::RightParen);
			return node;
		}
		throw std::runtime_error("can't parse atom");
	}

	std::unique_ptr<ExpressionAST> ParseMulDiv()
	{
		auto node = ParseAtom();
		while (m_token.type == Token::Mul || m_token.type == Token::Div)
		{
			const auto op = m_token;
			if (m_token.type == Token::Mul)
			{
				Eat(Token::Mul);
			}
			else if (m_token.type == Token::Div)
			{
				Eat(Token::Div);
			}
			node = std::make_unique<BinaryExpressionAST>(std::move(node), ParseAtom(),
				op.type == Token::Mul ? '*' : '/');
		}
		return node;
	}

	std::unique_ptr<ExpressionAST> ParseAddSub()
	{
		auto node = ParseMulDiv();
		while (m_token.type == Token::Plus || m_token.type == Token::Minus)
		{
			const auto op = m_token;
			if (m_token.type == Token::Plus)
			{
				Eat(Token::Plus);
			}
			else if (m_token.type == Token::Minus)
			{
				Eat(Token::Minus);
			}
			node = std::make_unique<BinaryExpressionAST>(std::move(node), ParseMulDiv(),
				op.type == Token::Plus ? '+' : '-');
		}
		return node;
	}

private:
	void Eat(Token::Type type)
	{
		if (m_token.type != type)
		{
			throw std::runtime_error("can't eat that token");
		}
		m_token = m_lexer->GetNextToken();
	}

private:
	std::unique_ptr<Lexer> m_lexer;
	Token m_token;
};

int main()
{
	std::string line;
	while (std::cout << ">>> " && getline(std::cin, line))
	{
		try
		{
			auto generator = std::make_unique<SimpleCodeGenerator>();
			auto parser = std::make_unique<Parser>(std::make_unique<Lexer>(line));
			auto ast = parser->ParseAddSub();
			std::cout << generator->Generate(*ast) << std::endl;
		}
		catch (...)
		{
			std::cerr << "invalid expression" << std::endl;
		}
	}

	return 0;
}
