#pragma once

inline const int BATTERY_P_SIZE = 21;
inline const int BATTERY_P_GLOBAL[BATTERY_P_SIZE]  = {
    //4200,4150,4110,4080,4020,3980,3950,3910,3870,3850,3840,3820,3800,3790,3770,3750,3730,3710,3690,3610,3270
        //335,357,369,371,373,375,377,379,380,382,384,385,387,391,395,398,402,408,411,415,419 //should be 421//good enough
        335,350,361,368,371,374,377,379,380,382,384,385,387,391,395,397,401,407,410,414,418 //should be 421
    //327,361,369,371,373,375,377,379,380,382,384,385,387,391,395,398,402,408,411,415,420
    //327,357,364,366,368,370,372,373,374,376,378,379,381,384,388,390,394,399,402,406,410, 

};
int BATTERY_P_LOOKUP[BATTERY_P_SIZE];
/*int BATTERY_P_LOOKUP[BATTERY_P_SIZE] = {
    //-10
    //5,4,3,6,4,3 : 25
    //2,2,1,5,3,3 : 16
    327,361,369,371,373,375,377,379,380,382,384,385,387,391,395,398,401,405,407,409,411
};*/
//int BATTERY_P_SIZE = //sizeof(BATTERY_P_LOOKUP) / sizeof(BATTERY_P_LOOKUP[0]); //shoudl be 20 but idk


//couldnt fucking get this to run so yeah
/*int nearest(int target, const int arr[], int size) {
    // Sort the array (if not already sorted)
    std::sort(arr, arr + size);

    // Use lower_bound to find the first element >= target
    auto it = std::lower_bound(arr, arr + size, target);

    // Handle edge cases
    if (it == arr) return arr[0];  // Target is smaller than all elements
    if (it == arr + size) return arr[size - 1];  // Target is larger than all elements

    // Compare the found element and the previous element
    int next = *it;
    int prev = *(it - 1);

    // Return the nearest of the two
    return (abs(target - prev) <= abs(target - next)) ? prev : next;
}*/

