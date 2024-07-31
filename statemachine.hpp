#ifndef STATEMACHINE_HPP
#define STATEMACHINE_HPP
#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
enum problem {
    problem_empty,
    problem_1,
    problem_2,
    problem_3,
    problem_4
};

extern enum problem now_problem; // 在离开菜单后不变

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
enum action {
    reach_blackline_end,
    reach_blackline_start,
    data_collect_finish
};

// A- - - - - - - - -B

// D- - - - - - - - -C
namespace sp1 {
enum state_problem1 {
    wait_at_A,        // 在A等待并且确定目标偏航角
    run_straight_A2B, // 从A行驶到B
    stop_at_B,        // 到达B，进行停车并声光提示
};
}
namespace sp2 {
enum state_problem2 {
    wait_at_A,        // 在A等待并且确定目标偏航角
    run_straight_A2B, // 从A行驶到B,直线
    reach_B,          // 到达B，进行声光提示
    run_circle_B2C,
    wait_at_C,
    run_straight_C2D,
    reach_D,
    run_circle_D2A,
    stop_at_A
};
}
namespace sp3 {
enum state_problem3 {
    wait_at_A,        // 在A等待并且确定目标偏航角
    run_diagonal_A2C, // 从A行驶到C,对角线,行驶到A，车头指向B时的偏航角减去arctan
    reach_C,          // 到达C，进行声光提示
    run_reverse_circle_C2B,
    wait_at_B,
    run_diagonal_B2D, // 从B行驶到D,对角线,行驶到B，车头指向A时的偏航角加上arctan
    reach_D,
    run_circle_D2A,
    stop_at_A
};
}
namespace sp4 {
enum state_problem4 {
    wait_at_A,       // 在A等待并且确定目标偏航角
    run_from_A_to_C, // 从A行驶到B,直线
    reach_C,         // 到达B，进行声光提示
    run_C2B,
    wait_at_B,
    run_B2D,
    reach_D,
    run_D2A,
    stop_at_A
}; // 还有一个计数器}
}
class state_machine {
private:
    sp1::state_problem1 m_sp1;
    sp2::state_problem2 m_sp2;
    sp3::state_problem3 m_sp3;
    sp4::state_problem4 m_sp4;
    uint16_t m_sp4_round_count;
    const problem m_problem;
    void sp1_action_performed(const action x);
    void sp2_action_performed(const action x);
    void sp3_action_performed(const action x);
    void sp4_action_performed(const action x);

public:
    state_machine();
    sp1::state_problem1 get_sp1() const;
    sp2::state_problem2 get_sp2() const;
    sp3::state_problem3 get_sp3() const;
    sp4::state_problem4 get_sp4() const;
    uint16_t get_sp4_round_count() const;
    void action_performed(const action x);
    bool need_stop() const;
};
#endif
#endif