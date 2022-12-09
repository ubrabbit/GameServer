
void test_callback(int fd, short event, void *arg)
{
    if (event & EV_TIMEOUT)
    {
        LOGINFO("timeout");
        GameEventListener::Instance().TriggerEvent(E_GAME_EVENT_TYPE_TEST);
    } else if (event & EV_READ)
    {
        LOGINFO("trigger\n");
    }
}

int main()
{
    GameEventListener::Instance().RegistEvent(E_GAME_EVENT_TYPE_TEST, -1, 5, test_callback);
}
