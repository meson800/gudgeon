#pragma once
namespace Events {

enum Category
{
    Invalid = 0,
    Netework,
    UI,
    MockUI
};

enum Network
{
    Connected
};

enum MockUIEvents
{
    Key
};
}
