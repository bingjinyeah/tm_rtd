#include "stm8s.h"
#include "modbus_driver.h"


extern uint16_t MB_InReg[];
extern uint16_t adc_value;
uint16_t adc_value_last = 0;
float resistance_value = 0;
float temperature_value;

//table from -50C to 179 C
const float temperature_resistance[] = {
    803.063,  807.033,  811.003,  814.970,  818.937,  822.902,  826.865,  830.828,  834.789,  838.748, 
    842.707,  846.663,  850.619,  854.573,  858.526,  862.478,  866.428,  870.377,  874.325,  878.271, 
    882.217,  886.160,  890.103,  894.044,  897.985,  901.923,  905.861,  909.797,  913.732,  917.666, 
    921.599,  925.530,  929.461,  933.390,  937.317,  941.244,  945.169,  949.093,  953.016,  956.938, 
    960.859,  964.778,  968.696,  972.613,  976.529,  980.444,  984.358,  988.270,  992.181,  996.091, 
    1000.000, 1003.908, 1007.814, 1011.720, 1015.624, 1019.527, 1023.429, 1027.330, 1031.229, 1035.128, 
    1039.025, 1042.921, 1046.816, 1050.710, 1054.603, 1058.495, 1062.385, 1066.274, 1070.162, 1074.049, 
    1077.935, 1081.820, 1085.703, 1089.585, 1093.467, 1097.347, 1101.225, 1105.103, 1108.980, 1112.855, 
    1116.729, 1120.602, 1124.474, 1128.345, 1132.215, 1136.083, 1139.950, 1143.817, 1147.681, 1151.545,  
    1155.408, 1159.270, 1163.130, 1166.989, 1170.847, 1174.704, 1178.560, 1182.414, 1186.268, 1190.120, 
    1193.971, 1197.821, 1201.670, 1205.518, 1209.364, 1213.210, 1217.054, 1220.897, 1224.739, 1228.579,  
    1232.419, 1236.257, 1240.095, 1243.931, 1247.766, 1251.600, 1255.432, 1259.264, 1263.094, 1266.923, 
    1270.751, 1274.578, 1278.404, 1282.228, 1286.052, 1289.874, 1293.695, 1297.515, 1301.334, 1305.152,  
    1308.968, 1312.783, 1316.597, 1320.411, 1324.222, 1328.033, 1331.843, 1335.651, 1339.458, 1343.264, 
    1347.069, 1350.873, 1354.676, 1358.477, 1362.277, 1366.077, 1369.875, 1373.671, 1377.467, 1381.262, 
    1385.055, 1388.847, 1392.638, 1396.428, 1400.217, 1404.005, 1407.791, 1411.576, 1415.360, 1419.143, 
    1422.925, 1426.706, 1430.485, 1434.264, 1438.041, 1441.817, 1445.592, 1449.366, 1453.138, 1456.910, 
    1460.680, 1464.449, 1468.217, 1471.984, 1475.750, 1479.514, 1483.277, 1487.040, 1490.801, 1494.561, 
    1498.319, 1502.077, 1505.833, 1509.589, 1513.343, 1517.096, 1520.847, 1524.598, 1528.347, 1532.096, 
    1535.843, 1539.589, 1543.334, 1547.078, 1550.820, 1554.562, 1558.302, 1562.041, 1565.779, 1569.516, 
    1573.251, 1576.986, 1580.719, 1584.451, 1588.182, 1591.912, 1595.641, 1599.368, 1603.095, 1606.820, 
    1610.544, 1614.267, 1617.989, 1621.709, 1625.429, 1629.147, 1632.864, 1636.580, 1640.295, 1644.009,  
    1647.721, 1651.433, 1655.143, 1658.852, 1662.560, 1666.267, 1669.972, 1673.677, 1677.380, 1681.082 
};

float get_temperature(float data)
{
    uint16_t length = sizeof(temperature_resistance)/sizeof(float);
    uint16_t head;
    uint16_t term;
    float    tmp0,tmp1;
    
    if(data < temperature_resistance[0]){
        return -100;
    }
    if(data > temperature_resistance[length-1]){
        return 500;
    }
    head = 0;
    term = head + length;
    while(length > 1){
        if(data < temperature_resistance[head + (length+1)/2]){
            term = head + (length+1)/2;
            length = term - head;
        }else{
            head = head + (length+1)/2;
            length = term - head;
        }
    }
    tmp0 = temperature_resistance[term] - temperature_resistance[head];
    tmp1 = data - temperature_resistance[head];
    temperature_value = head - 50 + tmp1/tmp0;
    return temperature_value;
}

void rtd_task()
{
    float tmp;
    
    if(adc_value_last!=adc_value){
        adc_value_last = adc_value;
        //*5 is 5V ref voltage, 15 is the scale factor,tmp is the vol diff
        //tmp = (adc_value / 1023.0) * 5 / 15.0;
        tmp = adc_value / 3069.0;
        tmp = 5 / (0.379 + tmp) - 1;
        //when adc_value=0,rtd_value=820;when adc_value=1023,rtd_value=1660;
        resistance_value = 10000 / tmp;
        temperature_value = get_temperature(resistance_value);
        MB_InReg[0] = (uint16_t)(temperature_value * 10);
    }
}