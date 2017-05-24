function executer(way_points)
    state_start = way_points(1,:);

    goal_flag = false;
    i_w = 1;
    state_current = state_start;
    stateL = state_start;
    stateR = state_start;
    dt = 0.1;
    while ~goal_flag
        errorTotal = 0;
        errorLast = 0;
       % send first way point to controller
       [pwm, error, errorTotal] = controller(way_points(i_w,:), state_current, errorTotal, errorLast);
       % convert pwm to acceleration
       [aL, aR] = pwm2torque(pwm);
       % acceleration to dynamics
       stateR(3) = aR;
       stateL(3) = aL;
       [stateL, stateR, state_current] = dynamics(stateL, stateR, state_current, dt);
       % check state against goal and update as necessary
       if sum(abs(state_current - way_points(i_w,:))) < 1
           i_w = i_w + 1;
           fprintf('Waypoint %s Started \n', i_w);
           if i_w > size(way_points,1) %stop when you have reached goal
               goal_flag = true;
           end
       end
    end
end