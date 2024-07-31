#include "statemachine.hpp"
problem now_problem = problem_4;
state_machine::state_machine() :
    m_problem(now_problem), m_sp4_round_count(0) {
}
sp1::state_problem1 state_machine::get_sp1() const {
    return m_sp1;
}

sp2::state_problem2 state_machine::get_sp2() const {
    return m_sp2;
}

sp3::state_problem3 state_machine::get_sp3() const {
    return m_sp3;
}

sp4::state_problem4 state_machine::get_sp4() const {
    return m_sp4;
}

uint16_t state_machine::get_sp4_round_count() const {
    return m_sp4_round_count;
}
void state_machine::action_performed(const action x) {
    if (m_problem == problem_1) {
        sp1_action_performed(x);
    } else if (m_problem == problem_2) {
        sp2_action_performed(x);
    } else if (m_problem == problem_3) {
        sp3_action_performed(x);
    } else if (m_problem == problem_4) {
        sp4_action_performed(x);
    }
}

void state_machine::sp1_action_performed(const action x) {
    using namespace sp1;
    if (get_sp1() == wait_at_A) {
        if (x == data_collect_finish) m_sp1 = run_straight_A2B;
    } else if (get_sp1() == run_straight_A2B) {
        if (x == reach_blackline_start) {
            m_sp1 = stop_at_B;
        }
    }
}
void state_machine::sp2_action_performed(const action x) {
}
void state_machine::sp3_action_performed(const action x) {
}
void state_machine::sp4_action_performed(const action x) {
}
