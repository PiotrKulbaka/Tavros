#include <common.test.hpp>

#include <tavros/core/utils/pp_lexer.hpp>

#include <vector>

using namespace tavros::core;

using tt = pp_token::token_type;

constexpr std::string_view to_str(tt type) noexcept
{
    using enum tt;

    switch (type)
    {
        case directive_hash:    return "directive_hash";
        case directive_name:    return "directive_name";
        case header_name:       return "header_name";
        case directive_end:     return "directive_end";
        case identifier:        return "identifier";
        case number:            return "number";
        case punctuator:        return "punctuator";
        case string_literal:    return "string_literal";
        case character_literal: return "character_literal";
        case error:             return "error";
        case end_of_source:     return "end_of_source";
    }

    return "<unknown token_type>";
}

struct row_col
{
    int32 row = 0;
    int32 col = 0;
};

class pp_lexer_test : public unittest_scope
{
public:
    void SetUp() override
    {
        unittest_scope::SetUp();
        tokens.reserve(128);
    }

    void TearDown() override
    {
        unittest_scope::TearDown();
    }

    void print_tokens()
    {
        for (auto& tok : tokens) {
            std::cout << "TokenType: " << to_str(tok.type()) << std::endl; 
            if (tok.type() != tt::end_of_source && tok.type() != tt::directive_end && tt::error != tok.type()) {
                std::cout << "  TokenStr: '" << tok.lexeme() << "'" << std::endl; 
            }
            if (tok.type() == tt::error) {
                std::cout << "  TokenError: '" << tok.error_string() << "'" << std::endl;
            }
        }
    }

    void lex_all(string_view src)
    {
        pp_lexer lexer(src.data(), src.data() + src.size());

        int i = 0;
        for (; i < 128; ++i)
        {
            auto tok = lexer.next_token();
            tokens.push_back(tok);

            if (tok.type() == tt::end_of_source) {
                ASSERT_TRUE(lexer.end_of_source());
                break;
            }

            ASSERT_FALSE(lexer.end_of_source());
        }

        ASSERT_LE(i, 128);
    }

    void test_types(std::span<tt> types)
    {
        ASSERT_EQ(types.size(), tokens.size());

        for (size_t i = 0; i < tokens.size(); ++i) {
            EXPECT_EQ(types[i], tokens[i].type());
        }

        EXPECT_FALSE(assert_was_called());
    }

    void test_lexemes(std::span<string_view> lexemes)
    {
        ASSERT_EQ(lexemes.size(), tokens.size());

        for (size_t i = 0; i < tokens.size(); ++i) {
            EXPECT_EQ(lexemes[i], tokens[i].lexeme());
        }

        EXPECT_FALSE(assert_was_called());
    }

    void test_positions(std::span<row_col> pos)
    {
        ASSERT_EQ(pos.size(), tokens.size());

        for (size_t i = 0; i < tokens.size(); ++i) {
            EXPECT_EQ(pos[i].row, tokens[i].row());
            EXPECT_EQ(pos[i].col, tokens[i].col());
        }

        EXPECT_FALSE(assert_was_called());
    }

    void test_lines(std::span<string_view> lines)
    {
        ASSERT_EQ(lines.size(), tokens.size());

        for (size_t i = 0; i < tokens.size(); ++i) {
            EXPECT_EQ(lines[i], tokens[i].line());
        }

        EXPECT_FALSE(assert_was_called());
    }

public:
    std::vector<pp_token> tokens;
};



TEST_F(pp_lexer_test, empty_source)
{
    lex_all("");

    tt tp[] = {tt::end_of_source};
    string_view tl[] = {""};
    string_view ln[] = {""};
    row_col rc[] = {{1, 1}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, identifiers_and_numbers)
{
    constexpr string_view str = "foo bar123 42";
    lex_all(str);

    tt tp[] = {tt::identifier, tt::identifier, tt::number, tt::end_of_source};
    string_view tl[] = {"foo", "bar123", "42", ""};
    string_view ln[] = {str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 5}, {1, 12}, {1, 14}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, identifiers_and_numbers_with_comments)
{
    lex_all("/*z\n\nc*/foo/*abc*/bar123/*cde*///asd\n42");
    constexpr string_view line3 = "c*/foo/*abc*/bar123/*cde*///asd";
    constexpr string_view line4 = "42";

    tt tp[] = {tt::identifier, tt::identifier, tt::number, tt::end_of_source};
    string_view tl[] = {"foo", "bar123", "42", ""};
    string_view ln[] = {line3, line3, line4, line4};
    row_col rc[] = {{3, 4}, {3, 14}, {4, 1}, {4, 3}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, preproc_directive)
{
    lex_all("\n#define FOO 123\n");
    constexpr string_view line2 = "#define FOO 123";

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::identifier, tt::number, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "define", "FOO", "123", "", ""};
    string_view ln[] = {line2, line2, line2, line2, line2, ""};
    row_col rc[] = {{2, 1}, {2, 2}, {2, 9}, {2, 13}, {2, 16}, {3, 1}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, preproc_directive_with_line_wrap)
{
    lex_all(" #  \\  \n define\\\n FOO\\\n 123");
    constexpr string_view line1 = " #  \\  ";
    constexpr string_view line2 = " define\\";
    constexpr string_view line3 = " FOO\\";
    constexpr string_view line4 = " 123";

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::identifier, tt::number, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "define", "FOO", "123", "", ""};
    string_view ln[] = {line1, line2, line3, line4, line4, line4};
    row_col rc[] = {{1, 2}, {2, 2}, {3, 2}, {4, 2}, {4, 5}, {4, 5}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, preproc_directive_with_line_wrap2)
{
    lex_all(" #  \\  \n define\\\n FOO\\\n 123");
    constexpr string_view line1 = " #  \\  ";
    constexpr string_view line2 = " define\\";
    constexpr string_view line3 = " FOO\\";
    constexpr string_view line4 = " 123";

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::identifier, tt::number, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "define", "FOO", "123", "", ""};
    string_view ln[] = {line1, line2, line3, line4, line4, line4};
    row_col rc[] = {{1, 2}, {2, 2}, {3, 2}, {4, 2}, {4, 5}, {4, 5}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, preproc_directive_with_line_wrap_and_comment)
{
    lex_all(
        "#define \\\n"
        " /*comment*/ FOO \\\n"
        "  /*c*/ 123"
    );
    
    constexpr string_view line1 = "#define \\";
    constexpr string_view line2 = " /*comment*/ FOO \\";
    constexpr string_view line3 = "  /*c*/ 123";

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::identifier, tt::number, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "define", "FOO", "123", "", ""};
    string_view ln[] = {line1, line1, line2, line3, line3, line3};
    row_col rc[] = {{1, 1}, {1, 2}, {2, 14}, {3, 9}, {3, 12}, {3, 12}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, preproc_directive_with_only_backslashes)
{
    lex_all(
        "#define A \\\n"
        " \\\n"
        "\t\\\n"
        "  123"
    );

    constexpr string_view line1 = "#define A \\";
    constexpr string_view line2 = " \\";
    constexpr string_view line3 = "\t\\";
    constexpr string_view line4 = "  123";

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::identifier, tt::number, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "define", "A", "123", "", ""};
    string_view ln[] = {line1, line1, line1, line4, line4, line4};
    row_col rc[] = {{1, 1}, {1, 2}, {1, 9}, {4, 3}, {4, 6}, {4, 6}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, not_a_directive_with_backslashes)
{
    lex_all(
        "foo \\\n"
        "bar \\\n"
        "\t\\\n"
        "  123"
    );

    constexpr string_view line1 = "foo \\";
    constexpr string_view line2 = "bar \\";
    constexpr string_view line3 = "\t\\";
    constexpr string_view line4 = "  123";

    tt tp[] = {tt::identifier, tt::punctuator, tt::identifier, tt::punctuator, tt::punctuator, tt::number, tt::end_of_source};
    string_view tl[] = {"foo", "\\", "bar", "\\", "\\", "123", ""};
    string_view ln[] = {line1, line1, line2, line2, line3, line4, line4};
    row_col rc[] = {{1, 1}, {1, 5}, {2, 1}, {2, 5}, {3, 2}, {4, 3}, {4, 6}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, preproc_directive_with_comments)
{
    lex_all("\n/*asdasd*/#/*asd123*/define/*1234afasdf*/FOO/*asdf*/123/*asdfasdf*/\n");
    constexpr string_view line2 = "/*asdasd*/#/*asd123*/define/*1234afasdf*/FOO/*asdf*/123/*asdfasdf*/";

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::identifier, tt::number, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "define", "FOO", "123", "", ""};
    string_view ln[] = {line2, line2, line2, line2, line2, ""};
    row_col rc[] = {{2, 11}, {2, 22}, {2, 42}, {2, 53}, {2, 56}, {3, 1}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, directive_end_line_number)
{
    lex_all("\n\n#dir\n\nabc\n\n");

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::directive_end, tt::identifier, tt::end_of_source};
    string_view tl[] = {"#", "dir", "", "abc", ""};
    string_view ln[] = {"#dir", "#dir", "#dir", "abc", ""};
    row_col rc[] = {{3, 1}, {3, 2}, {3, 5}, {5, 1}, {7, 1}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, quoted_header_name)
{
    constexpr string_view str = "#include \"my_header.hpp\"";
    lex_all(str);

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::header_name, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "include", "my_header.hpp", "", ""};
    string_view ln[] = {str, str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 2}, {1, 10}, {1, 25}, {1, 25}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, include_with_identifier)
{
    constexpr string_view str = "#include \"my_header.hpp\" //abcd \n\n\nabc ";
    lex_all(str);
    
    constexpr string_view line1 = "#include \"my_header.hpp\" //abcd ";
    constexpr string_view line4 = "abc ";

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::header_name, tt::directive_end, tt::identifier, tt::end_of_source};
    string_view tl[] = {"#", "include", "my_header.hpp", "", "abc", ""};
    string_view ln[] = {line1, line1, line1, line1, line4, line4};
    row_col rc[] = {{1, 1}, {1, 2}, {1, 10}, {1, 25}, {4, 1}, {4, 5}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, quoted_header_name_with_comments)
{
    constexpr string_view str = "/*asd*/#/*def*/include/*123*/\"my_header.hpp\"/*456*/";
    lex_all(str);

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::header_name, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "include", "my_header.hpp", "", ""};
    string_view ln[] = {str, str, str, str, str};
    row_col rc[] = {{1, 8}, {1, 16}, {1, 30}, {1, 52}, {1, 52}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, angle_brackets_header_name)
{
    constexpr string_view str = "#include <my_header.hpp>";
    lex_all(str);

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::header_name, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "include", "my_header.hpp", "", ""};
    string_view ln[] = {str, str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 2}, {1, 10}, {1, 25}, {1, 25}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, angle_brackets_header_name_with_comments)
{
    constexpr string_view str = "/*asd*/#/*asd123*/include /*1234*/ <my_header.hpp> // 1234";
    lex_all(str);

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::header_name, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "include", "my_header.hpp", "", ""};
    string_view ln[] = {str, str, str, str, str};
    row_col rc[] = {{1, 8}, {1, 19}, {1, 36}, {1, 59}, {1, 59}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, comment_inside_header_name)
{
    constexpr string_view str = "#include <my_/*123qweasd*/header.hpp>";
    lex_all(str);

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::header_name, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "include", "my_/*123qweasd*/header.hpp", "", ""};
    string_view ln[] = {str, str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 2}, {1, 10}, {1, 38}, {1, 38}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, include_without_header_name)
{
    constexpr string_view str = "#include";
    lex_all(str);

    tt tp[] = {tt::directive_hash, tt::directive_name, tt::error, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"#", "include", "", "", ""};
    string_view ln[] = {str, str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 2}, {1, 9}, {1, 9}, {1, 9}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, include_in_single_comment)
{
    constexpr string_view str = "//#include <file>";
    lex_all(str);

    tt tp[] = {tt::end_of_source};
    string_view tl[] = {""};
    string_view ln[] = {str};
    row_col rc[] = {{1, 18}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, include_in_multi_comment)
{
    constexpr string_view str = "/*#include <file>*/";
    lex_all(str);

    tt tp[] = {tt::end_of_source};
    string_view tl[] = {""};
    string_view ln[] = {str};
    row_col rc[] = {{1, 20}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, hash_after_identifier)
{
    constexpr string_view str = "foo #include <file>";
    lex_all(str);

    tt tp[] = {tt::identifier, tt::punctuator, tt::identifier, tt::punctuator, tt::identifier, tt::punctuator, tt::end_of_source};
    string_view tl[] = {"foo", "#", "include", "<", "file", ">", ""};
    string_view ln[] = {str, str, str, str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 5}, {1, 6}, {1, 14}, {1, 15}, {1, 19}, {1, 20}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, string_literal)
{
    constexpr string_view str = "\"hello\"";
    lex_all(str);

    tt tp[] = {tt::string_literal, tt::end_of_source};
    string_view tl[] = {"hello", ""};
    string_view ln[] = {str, str};
    row_col rc[] = {{1, 1}, {1, 8}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, string_literal_with_comments)
{
    lex_all("/*ASD\n123\n\n\n*/\"hello\"//\n\n/*123122\n3*/");
    constexpr string_view line5 = "*/\"hello\"//";
    constexpr string_view line8 = "3*/";

    tt tp[] = {tt::string_literal, tt::end_of_source};
    string_view tl[] = {"hello", ""};
    string_view ln[] = {line5, line8};
    row_col rc[] = {{5, 3}, {8, 4}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, string_literal_with_comments_inside)
{
    constexpr string_view str = " \"he/*asdasdasd123*/ll//o\" ";
    lex_all(str);

    tt tp[] = {tt::string_literal, tt::end_of_source};
    string_view tl[] = {"he/*asdasdasd123*/ll//o", ""};
    string_view ln[] = {str, str};
    row_col rc[] = {{1, 2}, {1, 28}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, unclosed_string_literal)
{
    constexpr string_view str = "\"hello";
    lex_all(str);

    tt tp[] = {tt::error, tt::end_of_source};
    string_view tl[] = {"", ""};
    string_view ln[] = {str, str};
    row_col rc[] = {{1, 7}, {1, 7}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, unclosed_string_literal_with_new_line)
{
    constexpr string_view str = "\"hello\n";
    lex_all(str);

    tt tp[] = {tt::error, tt::end_of_source};
    string_view tl[] = {"", ""};
    string_view ln[] = {"\"hello", ""};
    row_col rc[] = {{1, 7}, {2, 1}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, two_unclosed_string_literals)
{
    lex_all("\"hello\n\"world");

    tt tp[] = {tt::error, tt::error, tt::end_of_source};
    string_view tl[] = {"", "", ""};
    string_view ln[] = {"\"hello", "\"world", "\"world"};
    row_col rc[] = {{1, 7}, {2, 7}, {2, 7}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, string_literal_with_escape_seq)
{
    constexpr string_view str = "\"hello \\\"w\\tor\\nld\\\" \"";
    lex_all(str);

    tt tp[] = {tt::string_literal, tt::end_of_source};
    string_view tl[] = {"hello \\\"w\\tor\\nld\\\" ", ""};
    string_view ln[] = {str, str};
    row_col rc[] = {{1, 1}, {1, 23}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, string_literal_with_escape_at_end_of_line)
{
    constexpr string_view str = "\"hello\\\n ";
    lex_all(str);

    tt tp[] = {tt::error, tt::end_of_source};
    string_view tl[] = {"", ""};
    string_view ln[] = {"\"hello\\", " "};
    row_col rc[] = {{1, 8}, {2, 2}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, unclosed_string_literal_and_identifier)
{
    lex_all("\"hello\nworld");

    tt tp[] = {tt::error, tt::identifier, tt::end_of_source};
    string_view tl[] = {"", "world", ""};
    string_view ln[] = {"\"hello", "world", "world"};
    row_col rc[] = {{1, 7}, {2, 1}, {2, 6}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, character_literal)
{
    constexpr string_view str = "\'hello\'";
    lex_all(str);

    tt tp[] = {tt::character_literal, tt::end_of_source};
    string_view tl[] = {"hello", ""};
    string_view ln[] = {str, str};
    row_col rc[] = {{1, 1}, {1, 8}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, unclosed_character_literal)
{
    constexpr string_view str = "\'hello";
    lex_all(str);

    tt tp[] = {tt::error, tt::end_of_source};
    string_view tl[] = {"", ""};
    string_view ln[] = {str, str};
    row_col rc[] = {{1, 7}, {1, 7}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, unclosed_multiline_comment)
{
    lex_all("  123 /* com\nment ");

    tt tp[] = {tt::number, tt::error, tt::end_of_source};
    string_view tl[] = {"123", "", ""};
    string_view ln[] = {"  123 /* com", "ment ", "ment "};
    row_col rc[] = {{1, 3}, {2, 6}, {2, 6}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}


TEST_F(pp_lexer_test, unclosed_multiline_comment2)
{
    constexpr string_view str = "  123 /*/";
    lex_all(str);

    tt tp[] = {tt::number, tt::error, tt::end_of_source};
    string_view tl[] = {"123", "", ""};
    string_view ln[] = {str, str, str};
    row_col rc[] = {{1, 3}, {1, 10}, {1, 10}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, comment_with_a_new_line)
{
    lex_all("abc //def \\\nqwe\nzxc //comment\\\\\\\\");

    tt tp[] = {tt::identifier, tt::identifier, tt::end_of_source};
    string_view tl[] = {"abc", "zxc", ""};
    string_view ln[] = {"abc //def \\", "zxc //comment\\\\\\\\", "zxc //comment\\\\\\\\"};
    row_col rc[] = {{1, 1}, {3, 1}, {3, 18}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, punctuation_tokens)
{
    constexpr string_view str = "(){}[];,+-*/=!<>#";
    lex_all(str);

    tt tp[] = {
        tt::punctuator, tt::punctuator, tt::punctuator, tt::punctuator,
        tt::punctuator, tt::punctuator, tt::punctuator, tt::punctuator,
        tt::punctuator, tt::punctuator, tt::punctuator, tt::punctuator,
        tt::punctuator, tt::punctuator, tt::punctuator, tt::punctuator,
        tt::punctuator, tt::end_of_source};
    string_view tl[] = {"(", ")", "{", "}", "[", "]", ";", ",", "+", "-", "*", "/", "=", "!", "<", ">", "#", ""};
    string_view ln[] = {str, str, str, str, str, str, str, str, str, str, str, str, str, str, str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}, {1, 8}, {1, 9}, {1, 10}, {1, 11}, {1, 12}, {1, 13}, {1, 14}, {1, 15}, {1, 16}, {1, 17}, {1, 18}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, wrong_character)
{
    constexpr string_view str = "\x0001sdf";
    lex_all(str);

    tt tp[] = {
        tt::error, tt::identifier, tt::end_of_source};
    string_view tl[] = {"\x0001", "sdf", ""};
    string_view ln[] = {str, str, str};
    row_col rc[] = {{1, 1}, {1, 2}, {1, 5}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, wrong_character_in_middle)
{
    constexpr string_view str = "abc\x01\def ghi";
    lex_all(str);

    tt tp[] = {tt::identifier, tt::error, tt::identifier, tt::identifier, tt::end_of_source};
    string_view tl[] = {"abc", "\x01", "def", "ghi", ""};
    string_view ln[] = {str, str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 4}, {1, 5}, {1, 9}, {1, 12}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, wrong_character_before_newline_and_directive)
{
    constexpr string_view str = "\x01 foo\n#include <a.h>";
    constexpr string_view line1 = "\x01 foo";
    constexpr string_view line2 = "#include <a.h>";

    lex_all(str);

    tt tp[] = {tt::error, tt::identifier, tt::directive_hash, tt::directive_name, tt::header_name, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"\x01", "foo", "#", "include", "a.h", "", ""};
    string_view ln[] = {line1, line1, line2, line2, line2, line2, line2};
    row_col rc[] = {{1, 1}, {1, 3}, {2, 1}, {2, 2}, {2, 10}, {2, 15}, {2, 15}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}


TEST_F(pp_lexer_test, null_character)
{
    constexpr string_view str = "\x01 foo\n#include <a.h>";
    constexpr string_view line1 = "\x01 foo";
    constexpr string_view line2 = "#include <a.h>";

    lex_all(str);

    tt tp[] = {tt::error, tt::identifier, tt::directive_hash, tt::directive_name, tt::header_name, tt::directive_end, tt::end_of_source};
    string_view tl[] = {"\x01", "foo", "#", "include", "a.h", "", ""};
    string_view ln[] = {line1, line1, line2, line2, line2, line2, line2};
    row_col rc[] = {{1, 1}, {1, 3}, {2, 1}, {2, 2}, {2, 10}, {2, 15}, {2, 15}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, null_character_outside_string)
{
    using namespace std::literals::string_view_literals;
    constexpr string_view str = "ab\0cd"sv;
    lex_all(str);

    tt tp[] = {tt::identifier, tt::error, tt::identifier, tt::end_of_source};
    string_view tl[] = {"ab", "\0"sv, "cd", ""};
    string_view ln[] = {str, str, str, str};
    row_col rc[] = {{1, 1}, {1, 3}, {1, 4}, {1, 6}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}

TEST_F(pp_lexer_test, null_character_inside_string_literal)
{
    using namespace std::literals::string_view_literals;
    constexpr string_view str = "\"ab\0cd\""sv;
    lex_all(str);

    tt tp[] = {tt::string_literal, tt::end_of_source};
    string_view tl[] = {"ab\0cd"sv, ""};
    string_view ln[] = {str, str};
    row_col rc[] = {{1, 1}, {1, 8}};

    test_types(tp);
    test_lexemes(tl);
    test_lines(ln);
    test_positions(rc);
}
