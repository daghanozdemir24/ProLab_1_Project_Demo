package model.ground;

import config.GameConstants;
import engine.Grid;
import model.capability.CanFly;
import model.capability.CanJump;
import model.common.Position;
import model.robot.Robot;

public class CraterRock extends Ground {

	public CraterRock() {
		super('K');
	}

	@Override
	public boolean stepOn(Robot robot, Grid grid) {
		if(robot instanceof CanFly) {
			if(robot.spendEnergy(GameConstants.KRATERLI_KAYA_FLY_COST)) {
				return true;
			}
			return false;
		}
		
		if(robot instanceof CanJump) {
			Position landPosition = robot.getPosition().next(robot.getDirection()).next(robot.getDirection());
			
			if(!grid.isValidPosition(landPosition) || grid.isOccupiedByRobot(landPosition))
				return false;
			
			Ground landGround = grid.getGroundAt(landPosition);
			
			
			if(landGround == null || landGround instanceof Obstacle || landGround instanceof CraterRock || landGround instanceof MethaneLake)
				return false;
			
			if(robot.spendEnergy(GameConstants.KRATERLI_KAYA_JUMP_COST)) {
				robot.moveTo(landPosition);
				
				if(landGround instanceof DeliveryRamp) {
					landGround.stepOn(robot, grid);
				}
				return true;
			}
			
			return false;
		}
		
		return robot.spendEnergy(GameConstants.KRATERLI_KAYA_DEFAULT_COST);
	}

}

