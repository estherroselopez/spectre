// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "Framework/TestingFramework.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <initializer_list>

#include "DataStructures/MathWrapper.hpp"
#include "Framework/TestCreation.hpp"
#include "Framework/TestHelpers.hpp"
#include "Helpers/Time/TimeSteppers/LtsHelpers.hpp"
#include "Helpers/Time/TimeSteppers/TimeStepperTestUtils.hpp"
#include "Time/BoundaryHistory.hpp"
#include "Time/History.hpp"
#include "Time/Slab.hpp"
#include "Time/Time.hpp"
#include "Time/TimeStepId.hpp"
#include "Time/TimeSteppers/AdamsBashforth.hpp"
#include "Time/TimeSteppers/LtsTimeStepper.hpp"
#include "Time/TimeSteppers/TimeStepper.hpp"
#include "Utilities/ErrorHandling/Assert.hpp"
#include "Utilities/ForceInline.hpp"
#include "Utilities/Gsl.hpp"
#include "Utilities/Literals.hpp"
#include "Utilities/MakeWithValue.hpp"

SPECTRE_TEST_CASE("Unit.Time.TimeSteppers.AdamsBashforth", "[Unit][Time]") {
  for (size_t order = 1; order < 9; ++order) {
    CAPTURE(order);
    const TimeSteppers::AdamsBashforth stepper(order);
    TimeStepperTestUtils::check_multistep_properties(stepper);
    CHECK(stepper.monotonic());
    for (size_t start_points = 0; start_points < order; ++start_points) {
      CAPTURE(start_points);
      const double epsilon = std::max(std::pow(1e-3, start_points + 1), 1e-14);
      TimeStepperTestUtils::integrate_test(stepper, start_points + 1,
                                           start_points, 1., epsilon);
      TimeStepperTestUtils::integrate_test_explicit_time_dependence(
          stepper, start_points + 1, start_points, 1., epsilon);

      const double large_step_epsilon =
          std::clamp(1.0e3 * std::pow(2.0e-2, start_points + 1), 1e-14, 1.0);
      TimeStepperTestUtils::integrate_error_test(
          stepper, start_points + 1, start_points, 1.0, large_step_epsilon, 20,
          1.0e-4);
      TimeStepperTestUtils::integrate_error_test(
          stepper, start_points + 1, start_points, -1.0, large_step_epsilon, 20,
          1.0e-4);
    }
    TimeStepperTestUtils::check_convergence_order(stepper, {10, 30});
    TimeStepperTestUtils::check_dense_output(stepper, {10, 30}, 1, true);

    CHECK(stepper.order() == order);
    CHECK(stepper.error_estimate_order() == order - 1);

    TimeStepperTestUtils::stability_test(stepper);
  }

  const Slab slab(0., 1.);
  const Time start = slab.start();
  const Time mid = slab.start() + slab.duration() / 2;
  const Time end = slab.end();
  const auto can_change = [](const Time& first, const Time& second,
                             const Time& now) {
    const TimeSteppers::AdamsBashforth stepper(2);
    TimeSteppers::History<double> history(2);
    history.insert(TimeStepId(true, 0, first), 0., 0.);
    history.insert(TimeStepId(true, 2, second), 0., 0.);
    return stepper.can_change_step_size(TimeStepId(true, 4, now), history);
  };
  CHECK(can_change(start, mid, end));
  CHECK_FALSE(can_change(start, end, mid));
  CHECK(can_change(mid, start, end));
  CHECK_FALSE(can_change(mid, end, start));
  CHECK_FALSE(can_change(end, start, mid));
  CHECK_FALSE(can_change(end, mid, start));

  TestHelpers::test_factory_creation<TimeStepper, TimeSteppers::AdamsBashforth>(
      "AdamsBashforth:\n"
      "  Order: 3");
  TestHelpers::test_factory_creation<LtsTimeStepper,
                                     TimeSteppers::AdamsBashforth>(
      "AdamsBashforth:\n"
      "  Order: 3");

  TimeSteppers::AdamsBashforth ab4(4);
  test_serialization(ab4);
  test_serialization_via_base<TimeStepper, TimeSteppers::AdamsBashforth>(4_st);
  test_serialization_via_base<LtsTimeStepper, TimeSteppers::AdamsBashforth>(
      4_st);
  // test operator !=
  TimeSteppers::AdamsBashforth ab2(2);
  CHECK(ab4 != ab2);

  TimeStepperTestUtils::check_strong_stability_preservation(
      TimeSteppers::AdamsBashforth(1), 1.0);
}

SPECTRE_TEST_CASE("Unit.Time.TimeSteppers.AdamsBashforth.Variable",
                  "[Unit][Time]") {
  for (size_t order = 1; order < 9; ++order) {
    INFO(order);
    for (size_t start_points = 0; start_points < order; ++start_points) {
      INFO(start_points);
      const double epsilon = std::max(std::pow(1e-3, start_points + 1), 1e-14);
      TimeStepperTestUtils::integrate_variable_test(
          TimeSteppers::AdamsBashforth(order), start_points + 1, start_points,
          epsilon);
    }
  }
}

SPECTRE_TEST_CASE("Unit.Time.TimeSteppers.AdamsBashforth.Backwards",
                  "[Unit][Time]") {
  for (size_t order = 1; order < 9; ++order) {
    INFO(order);
    for (size_t start_points = 0; start_points < order; ++start_points) {
      INFO(start_points);
      const double epsilon = std::max(std::pow(1e-3, start_points + 1), 1e-14);
      TimeStepperTestUtils::integrate_test(
          TimeSteppers::AdamsBashforth(order), start_points + 1, start_points,
          -1., epsilon);
      TimeStepperTestUtils::integrate_test_explicit_time_dependence(
          TimeSteppers::AdamsBashforth(order), start_points + 1, start_points,
          -1., epsilon);
    }
  }

  const Slab slab(0., 1.);
  const Time start = slab.start();
  const Time mid = slab.start() + slab.duration() / 2;
  const Time end = slab.end();
  const auto can_change = [](const Time& first, const Time& second,
                             const Time& now) {
    const TimeSteppers::AdamsBashforth stepper(2);
    TimeSteppers::History<double> history(2);
    history.insert(TimeStepId(false, 0, first), 0., 0.);
    history.insert(TimeStepId(false, 2, second), 0., 0.);
    return stepper.can_change_step_size(TimeStepId(false, 4, now), history);
  };
  CHECK_FALSE(can_change(start, mid, end));
  CHECK_FALSE(can_change(start, end, mid));
  CHECK_FALSE(can_change(mid, start, end));
  CHECK(can_change(mid, end, start));
  CHECK_FALSE(can_change(end, start, mid));
  CHECK(can_change(end, mid, start));
}

namespace {
// Non-copyable double to verify that the boundary code is not making
// internal copies.
class NCd {
 public:
  NCd() = default;
  explicit NCd(double x) : x_(x) {}
  NCd(const NCd&) = delete;
  NCd(NCd&&) = default;
  NCd& operator=(const NCd&) = delete;
  NCd& operator=(NCd&&) = default;
  ~NCd() = default;

  const double& operator()() const { return x_; }
  double& operator()() { return x_; }

 private:
  double x_;
};

auto make_math_wrapper(const gsl::not_null<NCd*> x) {
  return ::make_math_wrapper(&(*x)());
}

auto make_math_wrapper(const NCd& x) { return ::make_math_wrapper(x()); }

// Random numbers
constexpr double c10 = 0.949716728952811;
constexpr double c11 = 0.190663110072823;
constexpr double c20 = 0.932407227651314;
constexpr double c21 = 0.805454101952822;
constexpr double c22 = 0.825876851406978;

// Test coupling for integrating using two drivers (multiplied together)
NCd quartic_coupling(const NCd& local, const NCd& remote) {
  return NCd(local() * remote());
}

// Test functions for integrating a quartic using the above coupling.
// The derivative of quartic_answer is the product of the other two.
double quartic_side1(double x) { return c10 + x * c11; }
double quartic_side2(double x) { return c20 + x * (c21 + x * c22); }
double quartic_answer(double x) {
  return x * (c10 * c20
              + x * ((c10 * c21 + c11 * c20) / 2
                     + x * ((c10 * c22 + c11 * c21) / 3
                            + x * (c11 * c22 / 4))));
}

void do_lts_test(const std::array<TimeDelta, 2>& dt) {
  // For general time steppers the boundary stepper cannot be run
  // without simultaneously running the volume stepper.  For
  // Adams-Bashforth methods, however, the volume contribution is zero
  // if all the derivative contributions are from the boundary, so we
  // can leave it out.

  const bool forward_in_time = dt[0].is_positive();
  const auto simulation_less = [forward_in_time](const Time& a, const Time& b) {
    return forward_in_time ? a < b : b < a;
  };

  const auto make_time_id = [forward_in_time](const Time& t) {
    return TimeStepId(forward_in_time, 0, t);
  };

  const Slab slab = dt[0].slab();
  Time t = forward_in_time ? slab.start() : slab.end();

  const size_t order = 4;
  TimeSteppers::AdamsBashforth ab4(order);

  TimeSteppers::BoundaryHistory<NCd, NCd, NCd> history{};
  {
    const Slab init_slab = slab.advance_towards(-dt[0]);

    for (int32_t step = 1; step <= 3; ++step) {
      {
        const Time now = t - step * dt[0].with_slab(init_slab);
        history.local().insert_initial(make_time_id(now), order,
                                       NCd(quartic_side1(now.value())));
      }
      {
        const Time now = t - step * dt[1].with_slab(init_slab);
        history.remote().insert_initial(make_time_id(now), order,
                                        NCd(quartic_side2(now.value())));
      }
    }
  }

  NCd y(quartic_answer(t.value()));
  Time next_check = t + dt[0];
  std::array<Time, 2> next{{t, t}};
  for (;;) {
    const auto side = static_cast<size_t>(
        std::min_element(next.cbegin(), next.cend(), simulation_less)
        - next.cbegin());

    if (side == 0) {
      history.local().insert(make_time_id(t), order,
                             NCd(quartic_side1(t.value())));
    } else {
      history.remote().insert(make_time_id(t), order,
                              NCd(quartic_side2(t.value())));
    }

    gsl::at(next, side) += gsl::at(dt, side);

    t = *std::min_element(next.cbegin(), next.cend(), simulation_less);

    ASSERT(not simulation_less(next_check, t), "Screwed up arithmetic");
    if (t == next_check) {
      ab4.add_boundary_delta(&y, history, dt[0], quartic_coupling);
      ab4.clean_boundary_history(make_not_null(&history));
      CHECK(y() == approx(quartic_answer(t.value())));
      if (t.is_at_slab_boundary()) {
        break;
      }
      next_check += dt[0];
    }
  }
}

void check_lts_vts() {
  const Slab slab(0., 1.);

  const auto make_time_id = [](const Time& t) {
    return TimeStepId(true, 0, t);
  };

  Time t = slab.start();

  const size_t order = 4;
  TimeSteppers::AdamsBashforth ab4(order);

  TimeSteppers::BoundaryHistory<NCd, NCd, NCd> history{};
  {
    const Slab init_slab = slab.retreat();
    const TimeDelta init_dt = init_slab.duration() / 4;

    // clang-tidy misfeature: warns about boost internals here
    for (int32_t step = 1; step <= 3; ++step) {  // NOLINT
      // clang-tidy misfeature: warns about boost internals here
      const Time now = t - step * init_dt;  // NOLINT
      history.local().insert_initial(make_time_id(now), order,
                                     NCd(quartic_side1(now.value())));
      history.remote().insert_initial(make_time_id(now), order,
                                      NCd(quartic_side2(now.value())));
    }
  }

  std::array<std::deque<TimeDelta>, 2> dt{{
      {slab.duration() / 2, slab.duration() / 4, slab.duration() / 4},
      {slab.duration() / 6, slab.duration() / 6, slab.duration() * 2 / 9,
            slab.duration() * 4 / 9}}};

  NCd y(quartic_answer(t.value()));
  Time next_check = t + dt[0][0];
  std::array<Time, 2> next{{t, t}};
  for (;;) {
    const auto side = static_cast<size_t>(
        std::min_element(next.cbegin(), next.cend()) - next.cbegin());

    if (side == 0) {
      history.local().insert(make_time_id(next[0]), order,
                             NCd(quartic_side1(next[0].value())));
    } else {
      history.remote().insert(make_time_id(next[1]), order,
                              NCd(quartic_side2(next[1].value())));
    }

    const TimeDelta this_dt = gsl::at(dt, side).front();
    gsl::at(dt, side).pop_front();

    gsl::at(next, side) += this_dt;

    if (*std::min_element(next.cbegin(), next.cend()) == next_check) {
      ab4.add_boundary_delta(&y, history, next_check - t, quartic_coupling);
      ab4.clean_boundary_history(make_not_null(&history));
      CHECK(y() == approx(quartic_answer(next_check.value())));
      if (next_check.is_at_slab_boundary()) {
        break;
      }
      t = next_check;
      next_check += dt[0].front();
    }
  }
}

void test_neighbor_data_required() {
  // Test is order-independent
  const TimeSteppers::AdamsBashforth stepper(4);
  const Slab slab(0.0, 1.0);
  CHECK(not stepper.neighbor_data_required(TimeStepId(true, 0, slab.start()),
                                           TimeStepId(true, 0, slab.start())));
  CHECK(not stepper.neighbor_data_required(TimeStepId(true, 0, slab.start()),
                                           TimeStepId(true, 0, slab.end())));
  CHECK(stepper.neighbor_data_required(TimeStepId(true, 0, slab.end()),
                                       TimeStepId(true, 0, slab.start())));

  CHECK(not stepper.neighbor_data_required(TimeStepId(false, 0, slab.end()),
                                           TimeStepId(false, 0, slab.end())));
  CHECK(not stepper.neighbor_data_required(TimeStepId(false, 0, slab.end()),
                                           TimeStepId(false, 0, slab.start())));
  CHECK(stepper.neighbor_data_required(TimeStepId(false, 0, slab.start()),
                                       TimeStepId(false, 0, slab.end())));
}
}  // namespace

SPECTRE_TEST_CASE("Unit.Time.TimeSteppers.AdamsBashforth.Boundary",
                  "[Unit][Time]") {
  test_neighbor_data_required();

  // No local stepping
  for (size_t order = 1; order < 9; ++order) {
    INFO(order);
    const TimeSteppers::AdamsBashforth stepper(order);
    for (size_t start_points = 0; start_points < order; ++start_points) {
      INFO(start_points);
      const double epsilon = std::max(std::pow(1e-3, start_points + 1), 1e-14);
      TimeStepperTestUtils::lts::test_equal_rate(stepper, start_points + 1,
                                                 start_points, epsilon, true);
      TimeStepperTestUtils::lts::test_equal_rate(stepper, start_points + 1,
                                                 start_points, epsilon, false);
    }
  }

  // Local stepping with constant step sizes
  const Slab slab(0., 1.);
  for (const auto& full : {slab.duration(), -slab.duration()}) {
    do_lts_test({{full / 4, full / 4}});
    do_lts_test({{full / 4, full / 8}});
    do_lts_test({{full / 8, full / 4}});
    do_lts_test({{full / 16, full / 4}});
    do_lts_test({{full / 4, full / 16}});
    do_lts_test({{full / 32, full / 4}});
    do_lts_test({{full / 4, full / 32}});

    // Non-nesting cases
    do_lts_test({{full / 4, full / 6}});
    do_lts_test({{full / 6, full / 4}});
    do_lts_test({{full / 5, full / 7}});
    do_lts_test({{full / 7, full / 5}});
    do_lts_test({{full / 5, full / 13}});
    do_lts_test({{full / 13, full / 5}});
  }

  // Local stepping with varying time steps
  check_lts_vts();

  // Dense output
  for (size_t order = 1; order < 9; ++order) {
    INFO(order);
    TimeStepperTestUtils::lts::test_dense_output(
        TimeSteppers::AdamsBashforth(order));
  }
}

SPECTRE_TEST_CASE("Unit.Time.TimeSteppers.AdamsBashforth.Reversal",
                  "[Unit][Time]") {
  const TimeSteppers::AdamsBashforth ab3(3);

  const auto f = [](const double t) {
    return 1. + t * (2. + t * (3. + t * 4.));
  };
  const auto df = [](const double t) { return 2. + t * (6. + t * 12.); };

  TimeSteppers::History<double> history{3};
  const auto add_history = [&df, &f, &history](const int64_t slab,
                                               const Time& time) {
    history.insert(TimeStepId(true, slab, time), f(time.value()),
                   df(time.value()));
  };
  const Slab slab(0., 1.);
  add_history(0, slab.start());
  add_history(0, slab.start() + slab.duration() * 3 / 4);
  add_history(1, slab.start() + slab.duration() / 3);
  double y = f(1. / 3.);
  ab3.update_u(make_not_null(&y), history, slab.duration() / 3);
  CHECK(y == approx(f(2. / 3.)));
}

SPECTRE_TEST_CASE("Unit.Time.TimeSteppers.AdamsBashforth.Boundary.Reversal",
                  "[Unit][Time]") {
  const size_t order = 3;
  const TimeSteppers::AdamsBashforth ab3(order);

  const auto f = [](const double t) {
    return 1. + t * (2. + t * (3. + t * 4.));
  };
  const auto df = [](const double t) { return 2. + t * (6. + t * 12.); };

  const Slab slab(0., 1.);
  TimeSteppers::BoundaryHistory<double, double, double> history{};
  const auto add_history = [&df, &history](const TimeStepId& time_id) {
    history.local().insert(time_id, order, df(time_id.step_time().value()));
    history.remote().insert(time_id, order, 0.);
  };
  add_history(TimeStepId(true, 0, slab.start()));
  add_history(TimeStepId(true, 0, slab.start() + slab.duration() * 3 / 4));
  add_history(TimeStepId(true, 1, slab.start() + slab.duration() / 3));
  double y = f(1. / 3.);
  ab3.add_boundary_delta(
      &y, history, slab.duration() / 3,
      [](const double local, const double /*remote*/) { return local; });
  CHECK(y == approx(f(2. / 3.)));
}
