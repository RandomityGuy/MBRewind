#include <TorqueLib/TGE.h>
#ifdef WIN32
#include "include/win/discord_rpc.h"
#else // Must be mac
#include "include/osx/discord_rpc.h"
#endif
#include <PluginLoader/PluginInterface.h>


void onReady(const DiscordUser* request)
{
    printf("Starting RPC for user %s", request->username);
    TGE::Con::printf("Starting RPC for user %s", request->username);
}

void onError(int errorCode, const char* message)
{
    printf("DISCORD RPC ERROR %d %s", errorCode, message);
    TGE::Con::errorf("DISCORD RPC ERROR %d %s", errorCode, message);
}

void onDisconnected(int errorCode, const char* message)
{
    printf("DISCORD RPC ERROR %d %s", errorCode, message);
    TGE::Con::errorf("DISCORD RPC ERROR %d %s", errorCode, message);
}

void InitDiscord()
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = onReady;
    handlers.errored = onError;
    handlers.disconnected = onDisconnected;
    handlers.joinGame = NULL;
    handlers.spectateGame = NULL;
    handlers.joinRequest = NULL;

    // Discord_Initialize(const char* applicationId, DiscordEventHandlers* handlers, int autoRegister, const char* optionalSteamId)
    Discord_Initialize("747396664813289534", &handlers, 0, nullptr);
}


void RefreshDiscord()
{
    Discord_UpdateConnection();
    Discord_RunCallbacks();
}

ConsoleFunction(initDiscordRPC, void, 1, 1, "initDiscordRPC()")
{
    InitDiscord();
    RefreshDiscord();
}

ConsoleFunction(shutdownDiscordRPC, void, 1, 1, "shutdownDiscordRPC()")
{
    RefreshDiscord();
    Discord_Shutdown();
}

ConsoleFunction(clearDiscordRPC, void, 1, 1, "clearDiscordRPC()")
{
    Discord_ClearPresence();
    RefreshDiscord();
}

ConsoleFunction(updateDiscordRPC, void, 1, 1, "updateDiscordRPC()")
{
    RefreshDiscord();
}

ConsoleFunction(setDiscordRPCLevel, void, 2, 2, "setDiscordRPCLevel(string level)")
{
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.state = argv[1];
    discordPresence.details = "Playing Level";
    discordPresence.largeImageKey = "mbp";
    Discord_UpdatePresence(&discordPresence);
    RefreshDiscord();
}

ConsoleFunction(setDiscordRPCReplay, void, 2, 2, "setDiscordRPCReplay(string level)")
{
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.state = argv[1];
    discordPresence.details = "Watching Replay";
    discordPresence.largeImageKey = "mbp";
    Discord_UpdatePresence(&discordPresence);
    RefreshDiscord();
}

ConsoleFunction(setDiscordRPCGuiText, void, 2, 2, "setDiscordRPCGuiText(string text)")
{
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.details = argv[1];
    discordPresence.largeImageKey = "mbp";
    Discord_UpdatePresence(&discordPresence);
    RefreshDiscord();
}

PLUGINCALLBACK void preEngineInit(PluginInterface* plugin)
{
    TGE::Con::printf("LOADED DISCORD RPC PRE");
}

PLUGINCALLBACK void postEngineInit(PluginInterface* plugin)
{
    TGE::Con::printf("LOADED DISCORD RPC POST");
    InitDiscord();
    RefreshDiscord();
}

PLUGINCALLBACK void engineShutdown(PluginInterface* plugin)
{
    TGE::Con::printf("SHUTDOWN DISCORD RPC");
    Discord_Shutdown();
}