OPS-SAT GROUND CONTROL CONSOLE - REPLACEMENT SOURCE SET

Copy these files into the root of your satellite-telemetry project:
  main.cpp, telemetry.hpp, structures.hpp, simulation.cpp,
  csv.cpp, network.hpp, network.cpp, CMakeLists.txt

Keep your existing data/telemetry_real.csv file in data/.

Build from the project root:
  rm -rf build
  cmake -S . -B build
  cmake --build build -j4

Run:
  ./build/satellite-telemetry

The CLI expects data/telemetry_real.csv relative to the current working directory.
The simulation replays source rows to reach 10,000 processing events. The CSV anomaly
column is intentionally ignored. Worker pods are logical C++ worker threads, not Kubernetes.
