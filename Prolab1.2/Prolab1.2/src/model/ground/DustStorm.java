package model.ground;

import config.GameConstants;
import engine.Grid;
import model.capability.CanFly;
import model.robot.Robot;

public class DustStorm extends Ground {

	public DustStorm() {
		super('T');
	}

	@Override
	public boolean stepOn(Robot robot, Grid grid) {
		if(robot instanceof CanFly) {
			robot.spendEnergy(GameConstants.TOZ_FIRTINASI_FLY_COST);
			return false;
		}
		return robot.spendEnergy(GameConstants.TOZ_FIRTINASI_DEFAULT_COST);
	}

}

