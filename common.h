#pragma once

const char g_open[6] = {(char)0x88, (char)0x88, (char)0x01, (char)0x00, (char)0x22, (char)0x22};
const char g_close[6] = {(char)0x88, (char)0x88, (char)0x01, (char)0x10, (char)0x22, (char)0x22};
const char g_hurt[6] = {(char)0x88, (char)0x88, (char)0xFF, (char)0xFF, (char)0x22, (char)0x22};
const char g_state[6] = {(char)0x88, (char)0x88, (char)0x02, (char)0x00, (char)0x22, (char)0x22};
const char g_state_re_open[8] = {(char)0x77, (char)0x77, (char)0x01, (char)0x00, (char)0x00, (char)0x00,(char)0x33,(char)0x33};
const char g_state_re_close[8] = {(char)0x77, (char)0x77, (char)0x01, (char)0x00, (char)0x10, (char)0x00,0x33,(char)0x33};