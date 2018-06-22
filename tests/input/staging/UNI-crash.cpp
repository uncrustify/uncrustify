UNIT_TEST_SUITE(CollabTracker)
{
    TEST(IsAssetsPath_WhenAssetPathWinSeparator_Returns_True)
    {
        CHECK(CollabTracker::IsAssetsPath(kTestAssetPath2));
    }

    TEST_FIXTURE(Fixture, IsPathTracked_WhenNotAssetsPath_Returns_False)
    {
        CHECK(!CollabTracker::IsPathTracked(kTestNotAssetPath));
    }

}
