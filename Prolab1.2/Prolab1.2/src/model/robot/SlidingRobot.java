package model.robot;

import model.capability.CanSlide;
import model.common.Position;

public class SlidingRobot extends Robot implements CanSlide {

	public SlidingRobot(int id, Position position, int energy) {
		super(id, "KIZAKLI", position, energy);
	}

}

