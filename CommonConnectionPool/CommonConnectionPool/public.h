#pragma once

// 对LOG日志打印进行封装
#define LOG(str) \
    cout << __FILE__ << ":" << __LINE__ << " " << \
    __TIMESTAMP__ << ":" << str << endl;