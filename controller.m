function [pwm, error, errorTotal] = controller(state_goal, state_current, errorTotal, errorLast)
    kP = 1;
    kI = 0;
    kD = 0;
    if nargin < 3
        errorTotal = 0;
    end
    if nargin < 4
        errorLast = 0;
    end
    error = sum(state_goal(1:2) - state_current(1:2)); %error in position
    derror = error - errorLast;
    errorTotal = errorTotal + error;
    
     
    
end