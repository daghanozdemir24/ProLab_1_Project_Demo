package engine;

import command.Command;
import model.common.RobotStatus;
import model.robot.Robot;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Stack;

public class SimulationManager {

    private final Grid grid;
    private final List<Command> commands;
    private final List<Robot> sortedRobots;

    private int commandIndex = 0;
    private int robotIndex = 0;
    private int totalStepsCompleted = 0;

    private final Stack<SimulationState> historyStack = new Stack<>();
    private String lastActionMessage = "Simülasyon hazır.";

    public SimulationManager(Grid grid, List<Command> commands) {
        this.grid = grid;
        this.commands = commands != null ? new ArrayList<>(commands) : new ArrayList<>();

        this.sortedRobots = new ArrayList<>(grid.getRobots());
        this.sortedRobots.sort(Comparator.comparingInt(Robot::getId));

        saveState("Başlangıç durumu");
    }

    private void saveState(String desc) {
        historyStack.push(new SimulationState(sortedRobots, commandIndex, robotIndex, desc));
    }

    public synchronized boolean stepForward() {
        if (isFinished()) {
            lastActionMessage = "Simülasyon tamamlandı.";
            return false;
        }

        Command currentCommand = commands.get(commandIndex);
        Robot currentRobot = sortedRobots.get(robotIndex);

        boolean executed = currentCommand.execute(currentRobot, grid);

        String cmdName = currentCommand.getClass().getSimpleName();
        lastActionMessage = "Komut " + (commandIndex + 1) + " (" + cmdName + ") - R" + currentRobot.getId() + " " +
                currentRobot.getGovdeTuru() + ": " + (currentRobot.getStatus() == RobotStatus.TESLIM ? "Paketi teslim etti!" :
                (currentRobot.getStatus() == RobotStatus.ENERJISIZ ? "Enerjisi tükendi!" : "Adım attı. Enerji: " + currentRobot.getEnergy()));

        totalStepsCompleted++;

        robotIndex++;
        if (robotIndex >= sortedRobots.size()) {
            robotIndex = 0;
            commandIndex++;
        }

        saveState(lastActionMessage);
        return true;
    }

    public synchronized boolean stepBackward() {
        if (historyStack.size() <= 1) {
            return false;
        }

        historyStack.pop();
        SimulationState prevState = historyStack.peek();

        for (Robot r : sortedRobots) {
            for (SimulationState.RobotSnapshot snap : prevState.getRobotSnapshots()) {
                if (snap.getId() == r.getId()) {
                    snap.restoreTo(r);
                    break;
                }
            }
        }

        this.commandIndex = prevState.getCurrentCommandIndex();
        this.robotIndex = prevState.getCurrentRobotIndex();
        this.lastActionMessage = "Geri alındı: " + prevState.getDescription();
        if (totalStepsCompleted > 0) totalStepsCompleted--;

        return true;
    }

    public void runAll() {
        while (!isFinished()) {
            stepForward();
        }
    }

    public boolean isFinished() {
        return commandIndex >= commands.size();
    }

    public Grid getGrid() {
        return grid;
    }

    public List<Command> getCommands() {
        return commands;
    }

    public int getDeliveredCount() {
        int count = 0;
        for (Robot r : sortedRobots) {
            if (r.getStatus() == RobotStatus.TESLIM) {
                count++;
            }
        }
        return count;
    }

    public int getTotalRemainingEnergy() {
        int total = 0;
        for (Robot r : sortedRobots) {
            total += r.getEnergy();
        }
        return total;
    }

    public int getCommandIndex() {
        return commandIndex;
    }

    public int getTotalCommands() {
        return commands.size();
    }

    public int getTotalStepsCompleted() {
        return totalStepsCompleted;
    }

    public int getTotalExpectedSteps() {
        return commands.size() * sortedRobots.size();
    }

    public String getLastActionMessage() {
        return lastActionMessage;
    }
}
