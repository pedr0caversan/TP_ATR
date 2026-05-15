#include <cstdlib>
#include <semaphore>
#include <thread>

#include "processes/mainProcess/CameraInspectionTask.h"
#include "processes/mainProcess/CeilingReconstructionTask.h"
#include "processes/mainProcess/DataColector.h"
#include "processes/mainProcess/DistanceComputationTask.h"
#include "processes/mainProcess/NavigationControlTask.h"

int main() {
    std::binary_semaphore x_was_sent{0};

    std::thread t1(cameraInspectionHandler);
    std::thread t2(ceilingReconstructionHandler, std::ref(x_was_sent));
    std::thread t3(distanceComputationHandler, std::ref(x_was_sent));
    std::thread t4(navigationControlHandler);
    std::thread t5(dataColectorHandler);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    return EXIT_SUCCESS;
}
