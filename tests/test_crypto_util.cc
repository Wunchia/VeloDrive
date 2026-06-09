#include "CryptoUtil.h"
#include <gtest/gtest.h>
#include <string>

using namespace std;

// ============================================================
//  generate_salt()
// ============================================================

TEST(CryptoUtilTest, GenerateSalt_DefaultLength)
{
    string salt = CryptoUtil::generate_salt();
    EXPECT_EQ(salt.size(), 8u);
}

TEST(CryptoUtilTest, GenerateSalt_CustomLength)
{
    string salt = CryptoUtil::generate_salt(16);
    EXPECT_EQ(salt.size(), 16u);
}

TEST(CryptoUtilTest, GenerateSalt_ContainsValidChars)
{
    string salt = CryptoUtil::generate_salt(100);
    for (char c : salt) {
        bool valid = (c >= '0' && c <= '9') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= 'A' && c <= 'Z');
        EXPECT_TRUE(valid) << "Unexpected char: " << c;
    }
}

TEST(CryptoUtilTest, GenerateSalt_Randomness)
{
    string s1 = CryptoUtil::generate_salt(16);
    string s2 = CryptoUtil::generate_salt(16);
    EXPECT_NE(s1, s2);
}

// ============================================================
//  hash_password()
// ============================================================

TEST(CryptoUtilTest, HashPassword_Produces64HexChars)
{
    string hash = CryptoUtil::hash_password("hello", "mysalt");
    EXPECT_EQ(hash.size(), 64u);
}

TEST(CryptoUtilTest, HashPassword_Deterministic)
{
    string h1 = CryptoUtil::hash_password("secret", "salt123");
    string h2 = CryptoUtil::hash_password("secret", "salt123");
    EXPECT_EQ(h1, h2);
}

TEST(CryptoUtilTest, HashPassword_DifferentPassword)
{
    string h1 = CryptoUtil::hash_password("passwordA", "salt");
    string h2 = CryptoUtil::hash_password("passwordB", "salt");
    EXPECT_NE(h1, h2);
}

TEST(CryptoUtilTest, HashPassword_DifferentSalt)
{
    string h1 = CryptoUtil::hash_password("mypwd", "saltA");
    string h2 = CryptoUtil::hash_password("mypwd", "saltB");
    EXPECT_NE(h1, h2);
}

TEST(CryptoUtilTest, HashPassword_EmptyInputs)
{
    string h1 = CryptoUtil::hash_password("", "salt");
    string h2 = CryptoUtil::hash_password("pwd", "");
    string h3 = CryptoUtil::hash_password("", "");
    EXPECT_EQ(h1.size(), 64u);
    EXPECT_EQ(h2.size(), 64u);
    EXPECT_EQ(h3.size(), 64u);
}

// ============================================================
//  generate_hashcode()
// ============================================================

TEST(CryptoUtilTest, GenerateHashcode_Produces64HexChars)
{
    const char *data = "hello world";
    string hash = CryptoUtil::generate_hashcode(data, 11);
    EXPECT_EQ(hash.size(), 64u);
}

TEST(CryptoUtilTest, GenerateHashcode_Deterministic)
{
    string data = "some file content";
    string h1 = CryptoUtil::generate_hashcode(data.c_str(), data.size());
    string h2 = CryptoUtil::generate_hashcode(data.c_str(), data.size());
    EXPECT_EQ(h1, h2);
}

TEST(CryptoUtilTest, GenerateHashcode_DifferentData)
{
    string h1 = CryptoUtil::generate_hashcode("abc", 3);
    string h2 = CryptoUtil::generate_hashcode("abd", 3);
    EXPECT_NE(h1, h2);
}

TEST(CryptoUtilTest, GenerateHashcode_EmptyData)
{
    string hash = CryptoUtil::generate_hashcode("", 0);
    EXPECT_EQ(hash.size(), 64u);
}

// ============================================================
//  generate_token() + verify_token() 往返
// ============================================================

TEST(CryptoUtilTest, TokenRoundtrip_Success)
{
    User original;
    original.id        = 42;
    original.username  = "testuser";
    original.createdAt = "2026-01-01 12:00:00";

    string token = CryptoUtil::generate_token(original);

    EXPECT_FALSE(token.empty());
    EXPECT_GT(token.size(), 10u);

    User recovered;
    bool ok = CryptoUtil::verify_token(token, recovered);
    EXPECT_TRUE(ok);

    EXPECT_EQ(recovered.id,        42);
    EXPECT_EQ(recovered.username,  "testuser");
    EXPECT_EQ(recovered.createdAt, "2026-01-01 12:00:00");
}

TEST(CryptoUtilTest, TokenVerification_InvalidToken)
{
    User user;
    bool ok = CryptoUtil::verify_token("garbage_token_not_valid", user);
    EXPECT_FALSE(ok);
}

TEST(CryptoUtilTest, TokenVerification_EmptyToken)
{
    User user;
    bool ok = CryptoUtil::verify_token("", user);
    EXPECT_FALSE(ok);
}
