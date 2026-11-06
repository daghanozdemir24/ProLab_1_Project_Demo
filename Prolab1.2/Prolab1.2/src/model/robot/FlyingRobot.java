package model.robot;

import model.capability.CanFly;
import model.common.Position;

public class FlyingRobot extends Robot implements CanFly {

	public FlyingRobot(int id, Position position, int energy) {
		super(id, "HAVALI", position, energy);
	}

}

