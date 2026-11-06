package model.robot;

import model.capability.CanJump;
import model.common.Position;

public class JumpingRobot extends Robot implements CanJump {

	public JumpingRobot(int id, Position position, int energy) {
		super(id, "SICRAYAN", position, energy);
	}

}

