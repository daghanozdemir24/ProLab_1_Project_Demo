package model.robot;

import model.common.Direction;
import model.common.Position;
import model.common.RobotStatus;

public abstract class Robot {
	
	private final int id;
	private final String typeOfBody;
	private Position position;
	private Direction direction;
	private int energy;
	private RobotStatus status;
	
	public Robot(int id, String typeOfBody, Position position, int energy) {
		super();
		this.id = id;
		this.typeOfBody = typeOfBody;
		this.position = position;
		this.direction = Direction.E;
		this.energy = energy;
		this.status = RobotStatus.AKTIF;
	}
	
	public void turnRight() {
		if(this.status == RobotStatus.AKTIF) {
			this.direction = direction.turnRight();
		}
	}
	
    public void turnLeft() { 
    		if(this.status == RobotStatus.AKTIF) {
    			this.direction = direction.turnLeft();
    		}
    }
    
    public boolean spendEnergy(int amount) {
    		
	    	if (this.status != RobotStatus.AKTIF) {
	            return false;
	        }
    		if(this.energy >= amount) {
    			this.energy -= amount;
    			return true;
    		}
    		else {
    			this.status = RobotStatus.ENERJISIZ;
    			return false;
    		}
    }
    public void setDelivered() { 
    		this.status = RobotStatus.TESLIM;
    }
    
    public void moveTo(Position newPos) { 
    		if(this.status != RobotStatus.ENERJISIZ) {
    			this.position = newPos;
    		}
    }
    
    public boolean isOperable() {
        return this.status == RobotStatus.AKTIF;
    }
	
    public int getId() { return id; }
    public String getGovdeTuru() { return typeOfBody; }
    public Position getPosition() { return position; }
    public Direction getDirection() { return direction; }
    public int getEnergy() { return energy; }
    public RobotStatus getStatus() { return status; }
	
    public String getReportLine() {
        return "ROBOT " + id + " " + typeOfBody + " KONUM " + position.getX() + " " + position.getY() 
                + " YON " + direction + " ENERJI " + energy + " DURUM " + status;
    }

	
	
}

