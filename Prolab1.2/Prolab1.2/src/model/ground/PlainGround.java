package model.ground;

import config.GameConstants;
import engine.Grid;
import model.robot.Robot;

public class PlainGround extends Ground {

	public PlainGround() {
		super('.');
	}

	@Override
	public boolean stepOn(Robot robot, Grid grid) {
		return robot.spendEnergy(GameConstants.DUZ_ZEMIN_COST);
	}

}

