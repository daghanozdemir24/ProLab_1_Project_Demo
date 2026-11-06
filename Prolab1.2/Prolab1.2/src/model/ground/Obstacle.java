package model.ground;

import engine.Grid;
import model.robot.Robot;

public class Obstacle extends Ground {

	public Obstacle() {
		super('X');
	}

	@Override
	public boolean stepOn(Robot robot, Grid grid) {
		return false;
	}

}

