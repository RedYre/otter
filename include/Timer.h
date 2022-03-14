#ifndef TIMER_H_
#define TIMER_H_

#include <iostream>
#include <chrono>
#include <string>

/*
 * used to measure time and log output
 */
class Timer
{
public:
  Timer(std::string setlog = "OTTER")
  {
    log = "[" + setlog + log;
  }

  // send a message and begin counting time
  void tick(std::string msg = "")
  {
    time = std::chrono::high_resolution_clock::now();
    message = msg;
    if (msg != "")
    {
      std::cout << log << msg << "\n";
    }
  }

  // send a message and write the measured time since last tick
  void tock(std::string msg)
  {
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(end - time);
    auto elapsed2 = std::chrono::duration_cast<std::chrono::seconds>(end - time);
    std::string time = std::to_string(elapsed.count()) + "min " +
                       std::to_string(elapsed2.count() % 60) + "sec";
    std::cout << log << msg << "(" << time << ")\n";
  }

  // repeat last message from tick write the measured time since last tick
  void tock()
  {
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(end - time);
    auto elapsed2 = std::chrono::duration_cast<std::chrono::seconds>(end - time);
    std::string time = std::to_string(elapsed.count()) + "min " +
                       std::to_string(elapsed2.count() % 60) + "sec";
    std::cout << log << message << " DONE "
              << "(" << time << ")\n";
  }

private:
  // members used for timerlogs
  std::chrono::high_resolution_clock::time_point time;
  std::string message = "";
  std::string log = "-TIMER] ";
};

#endif // TIMER_H_
