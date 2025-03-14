from SGP_Gen_Data import *

days = 14
t_end = seconds_in_day * days
st_min = 0.05  # millisecond

factor = 2
p_max = 193536000 #avoid MemoryError

init_step_size = 1

sat1_name = 'LEMUR2'
sat2_name = 'COSMOS'
with open(sat1_name + '.xml') as f:
    tle1 = next(omm.parse_xml(f))

with open(sat2_name + '.xml') as f:
    tle2 = next(omm.parse_xml(f))

filename = sat1_name + '_' + sat2_name

calculateAndSaveForAncasAndCatch(tle1, tle2, t_end, filename)
calculate_distance_for_graph(tle1, tle2, t_end,init_step_size, filename)
result = calculate_distance(tle1, tle2, t_end, st_min, p_max, factor, init_step_size, filename)
print("The minimal distance is:", result[0], "in time:", result[1])

