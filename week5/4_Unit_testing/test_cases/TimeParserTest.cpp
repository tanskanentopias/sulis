#include <gtest/gtest.h>
#include "../TimeParser.h"

TEST(TimeParserTest, ValidTime) {
    char time1[] = "000230";
    EXPECT_EQ(time_parse(time1), 150);

    char time2[] = "010000";
    EXPECT_EQ(time_parse(time2), 3600);

    char time3[] = "235959"; 
    EXPECT_EQ(time_parse(time3), 86399);

    char time4[] = "000000"; 
    EXPECT_EQ(time_parse(time4), 0);
}

TEST(TimeParserTest, RejectsUpperBoundaries) {
    char bad_seconds[] = "000060";
    EXPECT_EQ(time_parse(bad_seconds), TIME_VALUE_ERROR);

    char bad_minutes[] = "006000";
    EXPECT_EQ(time_parse(bad_minutes), TIME_VALUE_ERROR);

    char bad_hours[] = "240000";
    EXPECT_EQ(time_parse(bad_hours), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, RejectsNegativeBoundaries) {
    char neg_sec[] = "0000-5"; 
    EXPECT_EQ(time_parse(neg_sec), TIME_VALUE_ERROR);

    char neg_min[] = "00-500"; 
    EXPECT_EQ(time_parse(neg_min), TIME_VALUE_ERROR);

    char neg_hour[] = "-50000"; 
    EXPECT_EQ(time_parse(neg_hour), TIME_VALUE_ERROR);
}


TEST(TimeParserTest, HandlesNullInput) {
    EXPECT_EQ(time_parse(NULL), TIME_VALUE_ERROR);
}