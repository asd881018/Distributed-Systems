#include "core/utils.h"
#include <iomanip>
#include <iostream>
#include <stdlib.h>

#include <thread>
#include <vector>
#include <mutex>
#include <numeric>

#define sqr(x) ((x) * (x))
#define DEFAULT_NUMBER_OF_POINTS "12345678"

uint c_const = (uint)RAND_MAX + (uint)1;
inline double get_random_coordinate(uint *random_seed) {
  return ((double)rand_r(random_seed)) / c_const;
}

void get_points_in_circle(uint n, uint random_seed, uint &thread_circle_point) {
  uint circle_count = 0;
  double x_coord, y_coord;
  for (uint i = 0; i < n; i++) {
    x_coord = (2.0 * get_random_coordinate(&random_seed)) - 1.0;
    y_coord = (2.0 * get_random_coordinate(&random_seed)) - 1.0;
    if ((sqr(x_coord) + sqr(y_coord)) <= 1.0)
      circle_count++;
  }
  // return circle_count;
  thread_circle_point = circle_count;
}

void piCalculation(const uint n, const uint T) {
  timer serial_timer;
  double time_taken = 0.0;
  uint random_seed = 1;

  // serial_timer.start();
  // // Create threads and distribute the work across T threads
  // // -------------------------------------------------------------------
  // uint circle_points = get_points_in_circle(n, random_seed);
  // double pi_value = 4.0 * (double)circle_points / (double)n;
  // // -------------------------------------------------------------------
  // time_taken = serial_timer.stop();

  // My code:
  // -------------------------------------------------------------------
  uint total_pi_value = 0;
  const int thread_n = n/T;
  int remander = n % T;
  std::vector<uint> thread_circle_points(T);
  std::vector<std::thread> threads(T);
  std::vector<double> thread_time_taken(T);
  std::vector<int> thread_n_points(T, thread_n);
  uint circle_points = 0;
  timer thread_timer;

  serial_timer.start();

  // Create threads and distribute the work across T threads
  for (int i = 0; i < T; i++){
    random_seed += i;
    thread_timer.start();

    if (remander > 0){
      thread_n_points[i] += 1;
      threads[i] = std::thread(get_points_in_circle, thread_n_points[i], random_seed, std::ref(thread_circle_points[i]));
      remander --;
    }
    else
      threads[i] = std::thread(get_points_in_circle, thread_n_points[i], random_seed, std::ref(thread_circle_points[i]));
    
    thread_time_taken[i] = thread_timer.stop();
  }

  // uint circle_points = std::accumulate(thread_circle_points.begin(), thread_circle_points.end(), 0);

  for (int i = 0; i < T; i++){
    threads[i].join();
    circle_points += thread_circle_points[i];
  }

  const double pi_value = 4.0 * (double)circle_points / (double)n;

  time_taken = serial_timer.stop();
  // -------------------------------------------------------------------

  std::cout << "thread_id, points_generated, circle_points, time_taken\n";
  for (int i = 0; i < T; i++){
    std::cout << i << ", " << thread_n_points[i] << ", " << thread_circle_points[i] << ", " << thread_time_taken[i] << "\n";
  }
  // Print the above statistics for each thread
  // Example output for 2 threads:
  // thread_id, points_generated, circle_points, time_taken
  // 1, 100, 90, 0.12
  // 0, 100, 89, 0.12

  // Print the overall statistics
  std::cout << "Total points generated : " << n << "\n";
  std::cout << "Total points in circle : " << circle_points << "\n";
  std::cout << "Result : " << std::setprecision(VAL_PRECISION) << pi_value
            << "\n";
  std::cout << "Time taken (in seconds) : " << std::setprecision(TIME_PRECISION)
            << time_taken << "\n";
}

int main(int argc, char *argv[]) {
  // Initialize command line arguments
  cxxopts::Options options("pi_calculation",
                           "Calculate pi using serial and parallel execution");
  options.add_options(
      "custom",
      {
          {"nPoints", "Number of points",
           cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_POINTS)},
          {"nWorkers", "Number of workers",
           cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_WORKERS)},
      });

  auto cl_options = options.parse(argc, argv);
  uint n_points = cl_options["nPoints"].as<uint>();
  uint n_workers = cl_options["nWorkers"].as<uint>();
  std::cout << std::fixed;
  std::cout << "Number of points : " << n_points << "\n";
  std::cout << "Number of workers : " << n_workers << "\n";

  piCalculation(n_points, n_workers);

  return 0;
}
