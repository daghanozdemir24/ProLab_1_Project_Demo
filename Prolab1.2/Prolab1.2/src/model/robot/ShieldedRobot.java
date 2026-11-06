package model.robot;

import model.capability.CanFly;
import model.common.Position;
import model.capability.CanShield;

public class ShieldedRobot extends Robot implements CanShield  {

	public ShieldedRobot(int id, Position position, int energy) {
		super(id, "ZIRHLI", position, energy);
	}

}

