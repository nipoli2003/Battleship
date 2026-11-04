#include <gtest/gtest.h>
#include "model/PlayerName.hpp"
#include <filesystem>
#include <fstream>

namespace {
std::string tempPath(const char* name) {
    return (std::filesystem::temp_directory_path() / name).string();
}
} // namespace

TEST(PlayerNameTest, AcceptsNormalName) {
    EXPECT_FALSE(PlayerName::validate("Nelson").has_value());
    EXPECT_FALSE(PlayerName::validate("Yamamoto Isoroku").has_value());
}

TEST(PlayerNameTest, RejectsEmptyName) {
    EXPECT_TRUE(PlayerName::validate("").has_value());
}

TEST(PlayerNameTest, AcceptsMaxLengthAndRejectsLonger) {
    EXPECT_FALSE(PlayerName::validate(std::string(PlayerName::MAX_LEN, 'a')).has_value());
    EXPECT_TRUE(PlayerName::validate(std::string(PlayerName::MAX_LEN + 1, 'a')).has_value());
}

TEST(PlayerNameTest, RejectsLeadingOrTrailingSpace) {
    EXPECT_TRUE(PlayerName::validate(" Nelson").has_value());
    EXPECT_TRUE(PlayerName::validate("Nelson ").has_value());
}

TEST(PlayerNameTest, RejectsNonPrintableCharacters) {
    EXPECT_TRUE(PlayerName::validate("Nel\nson").has_value());
    EXPECT_TRUE(PlayerName::validate(std::string("Nel\x01son")).has_value());
}

TEST(PlayerNameTest, SameNameIgnoresCase) {
    EXPECT_TRUE(PlayerName::sameName("Nelson", "nELSON"));
    EXPECT_FALSE(PlayerName::sameName("Nelson", "Nimitz"));
    EXPECT_FALSE(PlayerName::sameName("Nelson", "Nelsons"));
}

TEST(PlayerNameTest, SavedNameRoundTrips) {
    const std::string path = tempPath("battleship_test_name_roundtrip.txt");
    ASSERT_TRUE(PlayerName::save("Nelson", path));
    auto loaded = PlayerName::loadSaved(path);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, "Nelson");
    std::filesystem::remove(path);
}

TEST(PlayerNameTest, MissingSaveFileGivesNoName) {
    EXPECT_FALSE(PlayerName::loadSaved(tempPath("battleship_test_name_missing.txt")).has_value());
}

TEST(PlayerNameTest, InvalidSavedNameIsIgnored) {
    const std::string path = tempPath("battleship_test_name_invalid.txt");
    {
        std::ofstream out(path);
        out << "this name is definitely longer than sixteen characters\n";
    }
    EXPECT_FALSE(PlayerName::loadSaved(path).has_value());
    std::filesystem::remove(path);
}
