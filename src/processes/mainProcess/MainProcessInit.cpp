#include "processes/mainProcess/MainProcessInit.hpp"

#include <cstdlib>
#include <semaphore>
#include <thread>

#include "processes/cameraInspection/CameraInspectionTask.hpp"
#include "processes/mainProcess/CeilingReconstructionTask.hpp"
#include "processes/mainProcess/DataColector.hpp"
#include "processes/mainProcess/DistanceComputationTask.hpp"
#include "processes/mainProcess/NavigationControlTask.hpp"
#include "utils/coord_buffer.hpp"
#include "utils/pos_buffer.hpp"
#include "utils/vel_buffer.hpp"

void mainProcessInit() {
    CoordBuffer coord_buf;
    PosBuffer pos_buf;
    VelBuffer vel_buf;
    std::binary_semaphore x_was_sent{0};
    std::binary_semaphore x_is_needed{0};
    std::binary_semaphore vel_was_sent{0};
    std::binary_semaphore vel_is_needed{0};

    std::thread t1(cameraInspectionHandler);
    std::thread t2(ceilingReconstructionHandler, std::ref(x_was_sent),
                   std::ref(x_is_needed), std::ref(pos_buf),
                   std::ref(coord_buf));
    std::thread t3(distanceComputationHandler, std::ref(x_was_sent),
                   std::ref(x_is_needed), std::ref(vel_was_sent),
                   std::ref(vel_is_needed), std::ref(pos_buf),
                   std::ref(vel_buf));
    std::thread t4(navigationControlHandler, std::ref(vel_was_sent),
                   std::ref(vel_is_needed), std::ref(vel_buf));
    std::thread t5(dataColectorHandler, std::ref(coord_buf));

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
}
