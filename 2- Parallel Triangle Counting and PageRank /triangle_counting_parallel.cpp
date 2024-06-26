#include "core/graph.h"
#include "core/utils.h"
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <thread>

long countTriangles(uintV *array1, uintE len1, uintV *array2, uintE len2,
                    uintV u, uintV v)
{

  uintE i = 0, j = 0; // indexes for array1 and array2
  long count = 0;

  if (u == v)
    return count;

  while ((i < len1) && (j < len2))
  {
    if (array1[i] == array2[j])
    {
      if ((array1[i] != u) && (array1[i] != v))
      {
        count++;
      }
      else
      {
        // triangle with self-referential edge -> ignore
      }
      i++;
      j++;
    }
    else if (array1[i] < array2[j])
    {
      i++;
    }
    else
    {
      j++;
    }
  }
  return count;
}

struct thread_status
{
  uint thread_id;
  long num_vertices;
  long num_edges;
  long triangle_count;
  double time_taken;
};

void printTriangleCountStatistics(const std::vector<thread_status> &thread_status, int n_workers, long triangle_count, double partitionTime, double time_taken)
{
  std::cout << "thread_id, num_vertices, num_edges, triangle_count, time_taken\n";
  for (uint t = 0; t < n_workers; t++)
  {
    std::cout << thread_status[t].thread_id << ", " << thread_status[t].num_vertices << ", " << thread_status[t].num_edges << ", " << thread_status[t].triangle_count << ", " << thread_status[t].time_taken << "\n";
    triangle_count += thread_status[t].triangle_count;
  }

  // Print the overall statistics
  std::cout << "Number of triangles : " << triangle_count << "\n";
  std::cout << "Number of unique triangles : " << triangle_count / 3 << "\n";
  std::cout << "Partitioning time (in seconds) : " << std::setprecision(TIME_PRECISION) << partitionTime << "\n";
  std::cout << "Time taken (in seconds) : " << std::setprecision(TIME_PRECISION)
            << time_taken << "\n";
}

void triangleCountSerial(Graph &g)
{
  uintV n = g.n_;
  long triangle_count = 0;
  double time_taken = 0.0;
  timer t1;
  t1.start();
  for (uintV u = 0; u < n; u++)
  {
    uintE out_degree = g.vertices_[u].getOutDegree();
    for (uintE i = 0; i < out_degree; i++)
    {
      uintV v = g.vertices_[u].getOutNeighbor(i);
      triangle_count += countTriangles(g.vertices_[u].getInNeighbors(),
                                       g.vertices_[u].getInDegree(),
                                       g.vertices_[v].getOutNeighbors(),
                                       g.vertices_[v].getOutDegree(), u, v);
    }
  }
  time_taken = t1.stop();
  std::cout << "Number of triangles : " << triangle_count << "\n";
  std::cout << "Number of unique triangles : " << triangle_count / 3 << "\n";
  std::cout << "Time taken (in seconds) : " << std::setprecision(TIME_PRECISION)
            << time_taken << "\n";
}

void triangleCountParallelStrategyOne(Graph &g, const uint &n_workers)
{
  uint n = g.n_;

  long triangle_count = 0;
  double time_taken = 0.0;
  double partitionTime = 0.0;
  timer total_time;
  timer thread_timer;

  const int num_vertices = n / n_workers;
  int remainder = n % n_workers;

  // The outNghs and inNghs for a given vertex are already sorted

  // Create threads and distribute the work across T threads
  // -------------------------------------------------------------------
  std::vector<std::thread> threads(n_workers);
  std::vector<thread_status> thread_status(n_workers);

  total_time.start();
  for (uint t = 0; t < n_workers; t++)
  {
    thread_timer.start();
    threads[t] = std::thread([&g, t, num_vertices, remainder, &thread_status, &n_workers, &partitionTime]()
                             {
            uintV n = g.n_;
            long triangle_count = 0;
            double time_taken = 0.0;
            timer t1, t2;
            t1.start();
            t2.start();
            uintV start = t * num_vertices;
            uintV end = (t + 1) * num_vertices;
            double t_partitionTime = t2.stop();
            uintE num_edges = 0;

            if (t == 0){
                partitionTime = t_partitionTime;
            }

            if (t == n_workers - 1)
            {
                end += remainder;
            }
            // Process each edge <u,v>
            for (uintV u = start; u < end; u++)
            {
                // For each outNeighbor v, find the intersection of inNeighbor(u) and
                // outNeighbor(v)
                uintE out_degree = g.vertices_[u].getOutDegree();
                num_edges += out_degree;
                for (uintE i = 0; i < out_degree; i++)
                {
                    uintV v = g.vertices_[u].getOutNeighbor(i);
                    triangle_count += countTriangles(g.vertices_[u].getInNeighbors(),
                                                     g.vertices_[u].getInDegree(),
                                                     g.vertices_[v].getOutNeighbors(),
                                                     g.vertices_[v].getOutDegree(), u, v);
                }
            }
            time_taken = t1.stop();
            thread_status[t] = {t,  end - start, num_edges, triangle_count, time_taken}; });
  }
  for (uint t = 0; t < n_workers; t++)
  {
    threads[t].join();
  }
  time_taken = total_time.stop();

  printTriangleCountStatistics(thread_status, n_workers, triangle_count, partitionTime, time_taken);
}

void triangleCountParallelStrategyTwo(Graph &g, const uint &n_workers)
{
  uintV n = g.n_;
  uint m = g.m_;

  long triangle_count = 0;
  double time_taken = 0.0;
  timer total_timer, thread_timer, partition_timer;

  int num_edges_per_thread = m / n_workers;
  int remainder = m % n_workers;

  std::vector<std::thread> threads(n_workers);
  std::vector<thread_status> thread_status(n_workers);

  std::vector<int> start_vertex(n_workers);
  std::vector<int> end_vertex(n_workers);

  uintV current_vertex = -1;
  uintV current_edges = 0;

  partition_timer.start();

  for (int i = 0; i < n_workers; i++)
  {

    if (i != n_workers - 1)
    {
      while (current_edges <= num_edges_per_thread)
      {
        current_vertex++;
        current_edges += g.vertices_[current_vertex].getOutDegree();
      }
      current_edges = 0;
      end_vertex[i] = current_vertex;
      current_vertex--;
    }
    else
    {
      end_vertex[i] = n;
    }
  }

  start_vertex[0] = 0;

  for (int i = 1; i < n_workers; i++)
  {
    start_vertex[i] = end_vertex[i - 1];
  }

  double partitionTime = partition_timer.stop();

  total_timer.start();
  for (uint t = 0; t < n_workers; t++)
  {
    threads[t] = std::thread([&g, t, &thread_status, &start_vertex, &end_vertex]()
                             {
            timer timerInThread;
            long triangle_count = 0;
            double time_taken = 0.0;
            uintE num_edges = 0;
            

             timerInThread.start();
            // Process each edge <u,v>
            for (uintV u = start_vertex[t]; u < end_vertex[t]; u++)
            {
                // For each outNeighbor v, find the intersection of inNeighbor(u) and outNeighbor(v)
                uintE out_degree = g.vertices_[u].getOutDegree();
                num_edges += out_degree;
                for (uintE i = 0; i < out_degree; i++)
                {
                    uintV v = g.vertices_[u].getOutNeighbor(i);
                    triangle_count += countTriangles(g.vertices_[u].getInNeighbors(),
                                                     g.vertices_[u].getInDegree(),
                                                     g.vertices_[v].getOutNeighbors(),
                                                     g.vertices_[v].getOutDegree(), u, v);
                }
            }
            time_taken = timerInThread.stop();
            thread_status[t] = {t,  0, num_edges, triangle_count, time_taken}; });
  }
  for (uint t = 0; t < n_workers; t++)
  {
    threads[t].join();
  }
  time_taken = total_timer.stop();

  printTriangleCountStatistics(thread_status, n_workers, triangle_count, partitionTime, time_taken);
}

void triangleCountParallelStrategyThree(Graph &g, const uint &n_workers)
{
  long triangle_count = 0;
  double time_taken = 0.0;
  double partitionTime = 0.0;
  timer total_time;
  timer thread_timer;

  uint n = g.n_;
  uint m = g.m_;

  // Define a function to get the next vertex to be processed
  std::mutex mtx; // Mutex to protect access to vertex processing
  uintV next_vertex = 0; // Initialize to process the first vertex

  std::vector<std::thread> threads (n_workers);
  std::vector<thread_status> thread_status(n_workers);

  total_time.start();
  for (uint t = 0; t < n_workers; t++)
  {
    threads[t] = std::thread([&g, t, &thread_status, &n_workers, &partitionTime, &next_vertex, &mtx]()
                             {
      uintV n = g.n_;
      long num_vertices = 0;
      long num_edges = 0;
      long local_triangle_count = 0;
      double time_taken = 0.0;
      timer t1;

      t1.start();
      while (true) {
        uintV v;
        {
          std::lock_guard<std::mutex> lock(mtx);
          v = next_vertex;
          next_vertex++;
        }

        if (v >= n) {
          break; // No more vertices to process
        }

        // Compute the number of triangles created by vertex v and its outNeighbors
        uintE out_degree = g.vertices_[v].getOutDegree();
        num_vertices++;
        num_edges += out_degree;

        for (uintE i = 0; i < out_degree; i++) {
          uintV u = g.vertices_[v].getOutNeighbor(i);
          local_triangle_count += countTriangles(g.vertices_[v].getInNeighbors(),
                                                g.vertices_[v].getInDegree(),
                                                g.vertices_[u].getOutNeighbors(),
                                                g.vertices_[u].getOutDegree(), v, u);
        }
      }

      // Store the thread's status
      time_taken = t1.stop();
      thread_status[t] = {t, num_vertices, num_edges, local_triangle_count, time_taken}; });
  }

  for (uint t = 0; t < n_workers; t++)
  {
    threads[t].join();
  }
  time_taken = total_time.stop();

  printTriangleCountStatistics(thread_status, n_workers, triangle_count, partitionTime, time_taken);
}

int main(int argc, char *argv[])
{
  cxxopts::Options options(
      "triangle_counting_serial",
      "Count the number of triangles using serial and parallel execution");
  options.add_options(
      "custom",
      {
          {"nWorkers", "Number of workers",
           cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_WORKERS)},
          {"strategy", "Strategy to be used",
           cxxopts::value<uint>()->default_value(DEFAULT_STRATEGY)},
          {"inputFile", "Input graph file path",
           cxxopts::value<std::string>()->default_value(
               "/scratch/input_graphs/roadNet-CA")},
      });

  auto cl_options = options.parse(argc, argv);
  uint n_workers = cl_options["nWorkers"].as<uint>();
  uint strategy = cl_options["strategy"].as<uint>();
  std::string input_file_path = cl_options["inputFile"].as<std::string>();
  std::cout << std::fixed;
  std::cout << "Number of workers : " << n_workers << "\n";
  std::cout << "Task decomposition strategy : " << strategy << "\n";

  Graph g;
  std::cout << "Reading graph\n";
  g.readGraphFromBinary<int>(input_file_path);
  std::cout << "Created graph\n";

  switch (strategy)
  {
  case 0:
    std::cout << "\nSerial\n";
    triangleCountSerial(g);
    break;
  case 1:
    std::cout << "\nVertex-based work partitioning\n";
    triangleCountParallelStrategyOne(g, n_workers);
    break;
  case 2:
    std::cout << "\nEdge-based work partitioning\n";
    triangleCountParallelStrategyTwo(g, n_workers);
    break;
  case 3:
    std::cout << "\nDynamic task mapping\n";
    triangleCountParallelStrategyThree(g, n_workers);
    break;
  default:
    break;
  }

  return 0;
}
