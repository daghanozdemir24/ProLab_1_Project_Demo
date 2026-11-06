package model.ground;

import config.GameConstants;
import engine.Grid;
import model.robot.Robot;

public class DeliveryRamp extends Ground {
    public DeliveryRamp() {
        super('H');
    }

    @Override
    public boolean stepOn(Robot robot, Grid grid) {
        if (robot.spendEnergy(GameConstants.TESLIM_RAMPASI_COST)) {
            robot.setDelivered(); 
            return true;
        }
        return false;
    }
}

