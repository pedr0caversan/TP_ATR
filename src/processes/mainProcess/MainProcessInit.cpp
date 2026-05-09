#include <cstdlib>
#include <thread>

#include "processes/mainProcess/CameraInspectionTask.h"
#include "processes/mainProcess/CeilingReconstructionTask.h"
#include "processes/mainProcess/DistanceComputationTask.h"
#include "processes/mainProcess/NavigationControlTask.h"

int main()
{
    std::thread t1(cameraInspectionHandler);
    std::thread t2(ceilingReconstructionHandler);
    std::thread t3(distanceComputationHandler);
    std::thread t4(navigationControlHandler);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    return EXIT_SUCCESS;
}
