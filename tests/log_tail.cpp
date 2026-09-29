#include "Harness.h"
#include "Logs/Tail.h"
#include <QBuffer>

int main() {
    return Tests::Run([] {
        QByteArray data;
        for (int line = 1; line <= 12; ++line) {
            data += QByteArray::number(line) + "\r\n";
        }
        data += "final unterminated line";
        QBuffer input(&data);
        input.open(QIODevice::ReadOnly);
        auto result = Bench::ReadLogTail(input, 10);
        if (result["totalLines"] != 13
            || result["lines"].size() != 10
            || result["lines"][0] != "4"
            || result["lines"].back() != "final unterminated line"
            || !result["truncated"].get<bool>()) {
            return 1;
        }
        input.seek(0);
        result = Bench::ReadLogTail(input, 1000000);
        if (result["returned"] != 13 || result["truncated"].get<bool>()) {
            return 2;
        }
        input.seek(0);
        result = Bench::ReadLogTail(input, 0);
        return result["lines"].empty() ? 0 : 3;
    });
}
