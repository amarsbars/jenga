function [way_points] = planner(state_goal, state_start)
    step_dist = 2;
    max_V = 1;
    % comes up with plan
    % just cutting it up into small pieces
    delta = state_goal - state_start;
    max_travel = sqrt(delta(1)^2 + delta(2)^2);
    steps = ceil(max_travel / step_dist);
    way_points = zeros(steps, length(state_goal));
    way_points(:,1) = linspace(state_start(1), state_goal(1), steps);
    way_points(:,2) = linspace(state_start(2), state_goal(2), steps);
    way_points(:,3) = linspace(state_start(3), state_goal(3), steps);
    way_points(2:end-1,3) = max_V; %all but the start and end go at max velocity
    way_points(:,4) = linspace(state_start(4), state_goal(4), steps);

end