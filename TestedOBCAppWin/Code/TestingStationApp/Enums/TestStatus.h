#ifndef TEST_STATUS_H
#define TEST_STATUS_H

enum class TestStatus {
    Queued,
    InProgress,
    Completed,
    Failed
};

#endif // TEST_STATUS_H