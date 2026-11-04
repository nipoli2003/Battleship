#include <gtest/gtest.h>
#include "model/AdmiralNames.hpp"
#include "model/PlayerName.hpp"

TEST(AdmiralNamesTest, ListsAreNotEmpty) {
    EXPECT_FALSE(AdmiralNames::creatorNames().empty());
    EXPECT_FALSE(AdmiralNames::joinerNames().empty());
}

TEST(AdmiralNamesTest, AllNamesAreValidPlayerNames) {
    for (auto name : AdmiralNames::creatorNames())
        EXPECT_FALSE(PlayerName::validate(std::string(name)).has_value()) << name;
    for (auto name : AdmiralNames::joinerNames())
        EXPECT_FALSE(PlayerName::validate(std::string(name)).has_value()) << name;
}

TEST(AdmiralNamesTest, ListsHaveNoNameInCommon) {
    for (auto c : AdmiralNames::creatorNames())
        for (auto j : AdmiralNames::joinerNames())
            EXPECT_FALSE(PlayerName::sameName(std::string(c), std::string(j))) << c;
}

TEST(AdmiralNamesTest, RandomNameComesFromTheRightList) {
    for (int i = 0; i < 50; ++i) {
        std::string c = AdmiralNames::randomCreatorName();
        std::string j = AdmiralNames::randomJoinerName();
        EXPECT_FALSE(PlayerName::validate(c).has_value());
        EXPECT_FALSE(PlayerName::validate(j).has_value());
        EXPECT_FALSE(PlayerName::sameName(c, j));
    }
}
