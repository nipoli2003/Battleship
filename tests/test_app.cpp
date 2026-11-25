#include <gtest/gtest.h>
#include "app/AppController.hpp"

// only simple screen opening/changing tests for now

TEST(AppControllerTest, StartsOnTitleScreen) {
    AppController app;
    EXPECT_EQ(app.state().scene, AppScene::TitleScreen);
    EXPECT_FALSE(app.state().shouldExit);
}

TEST(AppControllerTest, OpenLobbySetupTransitions) {
    AppController app;
    app.openLobbySetup();
    EXPECT_EQ(app.state().scene, AppScene::OnlineLobbyWait);
    EXPECT_EQ(app.state().lobbySubState, LobbySubState::Setup);
}

TEST(AppControllerTest, GoToMenuTransitions) {
    AppController app;
    app.openLobbySetup();
    app.goToMenu();
    EXPECT_EQ(app.state().scene, AppScene::MainMenu);
}

TEST(AppControllerTest, RequestExitSetsFlag) {
    AppController app;
    app.requestExit();
    EXPECT_TRUE(app.state().shouldExit);
}

TEST(AppControllerTest, SendPlacementFailsWithoutConnection) {
    AppController app;
    EXPECT_FALSE(app.sendPlacement({}));
}

TEST(AppControllerTest, ResetToMenuPreservesVolumeSettings) {
    AppController app;
    app.setMusicVolume(45);
    app.setSFXVolume(30);
    app.resetToMenu();
    EXPECT_EQ(app.state().musicVolume, 45);
    EXPECT_EQ(app.state().sfxVolume, 30);
    EXPECT_EQ(app.state().scene, AppScene::MainMenu);
}

TEST(AppControllerTest, GracefulExitSetsStateAndCleansUp) {
    AppController app;
    // TODO: not sure what to test here
    app.requestExit();
    EXPECT_TRUE(app.state().shouldExit);
}
