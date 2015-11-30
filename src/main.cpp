﻿
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <limits>
#include <functional>
#include <thread>

#include <QtWidgets/QApplication>
#include "qcustomplot.h"

struct ModelInputData
{
    double start_army_size;             // > 0          Tamanho inicial do exército
    double start_enemy_size;            // > 0          Tamanho inicial do exército inimigo
    double loose_battle_fraction;       // 0 < x < 1:   Porcentagem do exército que define derrota
    double army_skill;                  // 0 < x <= 1   Perícia do exército em eliminar inimigos
    double enemy_skill;                 // 0 < x <= 1   Perícia do inimigo em eliminar o exército
    double start_ammo;                  // > 0          Quantidade de munição que estará disponível durante a batalha
    double army_fire_rate;              // 0 < x <= 1   Taxa com que o exército consegue atirar a munição disponível
    double ammo_diffusion_coeffient;    // > 0          Velocidade com o que a munição é distribuída para o exército
    double formation_size;              // > 0          Tamanho da formação utilizada na batalha pelo exército
    double front_line_fraction;         // 0 < x <= 1   Porcentagem do exército que atuará na linha de frente
    double enemy_front_line_fraction;   // 0 < x <= 1   Porcentagem do exército inimigo que atuará na linha de frente

    double delta_time;                  // > 0
    double delta_x;                     // > 0

    /**
     * @brief ModelInputData default input data
     */
    ModelInputData()
    {
        start_army_size             = 1000.0;
        start_enemy_size            = 1000.0;
        loose_battle_fraction       = 0.05;
        army_skill                  = 0.05;
        enemy_skill                 = 0.01;
        start_ammo                  = 1000;
        army_fire_rate              = 0.9;
        ammo_diffusion_coeffient    = 1.0;
        formation_size              = 10;
        front_line_fraction         = 0.1;
        enemy_front_line_fraction   = 0.1;

        delta_time                  = 0.07;
        delta_x                     = 0.6;

        std::cout << "CFL = " << ammo_diffusion_coeffient * delta_time / (delta_x * delta_x) << std::endl;
    }


    friend std::ostream& operator<< (std::ostream& out, const ModelInputData& input_data)
    {
        out << "#-------------------------------------------------" << std::endl;
        out << "#--- GentlesmanBattle Input Data" << std::endl;
        out << "#-------------------------------------------------" << std::endl;
        out << "#- Start Army Size    : " << input_data.start_army_size << std::endl;
        out << "#- Start Enemy Size   : " << input_data.start_enemy_size << std::endl;
        out << "#- Army Skill         : " << input_data.army_skill << std::endl;
        out << "#- Enemy Skill        : " << input_data.enemy_skill << std::endl;
        out << "#- Start Ammo         : " << input_data.start_ammo << std::endl;
        out << "#- Ammo Diffusion Coef: " << input_data.ammo_diffusion_coeffient << std::endl;
        out << "#- Battle Field Size  : " << input_data.formation_size << std::endl;
        out << "#- Front Line Fraction: " << input_data.front_line_fraction << std::endl;
        out << "#- Delta Time         : " << input_data.delta_time << std::endl;
        out << "#- Delta X            : " << input_data.delta_x << std::endl;
        out << "#--------------------------------------------------" << std::endl;

        return out;
    }
};

struct ModelInfo
{
    double time             = 0.0;

    double new_army_size    = 0.0;
    double old_army_size    = 0.0;

    double new_enemy_size   = 0.0;
    double old_enemy_size   = 0.0;

    std::vector<double> new_ammo_amount;
    std::vector<double> old_ammo_amount;

    const ModelInputData& model_input;

    const double CFL = model_input.ammo_diffusion_coeffient * model_input.delta_time / (model_input.delta_x * model_input.delta_x);

    ModelInfo(const ModelInputData& input_data) :
        model_input(input_data)
    {
        // Setting up the mesh for the ammo diffusion
        new_ammo_amount.resize(input_data.formation_size / input_data.delta_x);
        old_ammo_amount.resize(new_ammo_amount.size());

        // Initial condition
        old_army_size = input_data.start_army_size;
        old_enemy_size = input_data.start_enemy_size;

        // Ammot starts at rear-line
        old_ammo_amount[1] = model_input.start_ammo;
    }

    void advance_time()
    {
        // Calculates the fraction of the army and enemies currently standing in the front-line
        double front_line_size = model_input.front_line_fraction * old_army_size;
        double enemy_front_line_size = model_input.front_line_fraction * old_enemy_size;

        // Army
        new_army_size = old_army_size - model_input.delta_time * model_input.enemy_skill * front_line_size * enemy_front_line_size;
        old_army_size = new_army_size;        

        // Enemy
        double shoots_fired = old_ammo_amount.back() * model_input.army_fire_rate;
        new_enemy_size = old_enemy_size - model_input.delta_time * model_input.army_skill * shoots_fired * front_line_size * old_enemy_size;
        old_enemy_size = new_enemy_size;

        // Ammo Diffusion        
        for(size_t i = 1; i < new_ammo_amount.size() - 1; ++i)
        {
            new_ammo_amount[i] = old_ammo_amount[i] + CFL * (old_ammo_amount[i-1] - 2.0 * old_ammo_amount[i] + old_ammo_amount[i+1]);
        }

        //
        // Ammo boundary conditions
        //

        // At x=0 there is no flow.
        new_ammo_amount[0] = new_ammo_amount[1];

        // At x=L the ammo is being used by the soldiers
        double ammo_usage_ratio = 1.0 - (1.0 / front_line_size); // explain this!!
        new_ammo_amount.back() = new_ammo_amount[new_ammo_amount.size() - 2] * ammo_usage_ratio;

        // Swap vectors for next time step
        new_ammo_amount.swap(old_ammo_amount);

        // Finally, advance the time
        time += model_input.delta_time;
    }

    /**
     * @brief True if the battle has came to an end
     *
     * The condition for the battle to stop is when the army size reaches a fraction defined
     * by @ref ModelInputData::loose_battle_fraction. The condition is applied for both the army
     * and the enemies.
     *
     * @return True if the battle has came to an end false otherwise
     */
    bool should_stop() const
    {        
        return new_army_size <= model_input.start_army_size * model_input.loose_battle_fraction ||
                new_enemy_size <= model_input.start_enemy_size * model_input.loose_battle_fraction;
    }

    /**
     * @brief Returns true if the number of soldiers is bigger than the number of enemies soldiers at this moment
     */
    bool is_army_winning() const
    {
        return old_army_size > old_enemy_size;
    }

    friend std::ostream& operator<< (std::ostream& out, const ModelInfo& info)
    {
        out << "#-------------------------------------------------" << std::endl;
        out << "#--- GentlesmanBattle Execution Summary" << std::endl;
        out << "#-------------------------------------------------" << std::endl;
        out << "# Number of Iterations      : " << info.time / info.model_input.delta_time << std::endl;
        out << "# Total time                : " << info.time << std::endl;
        out << "# Soldiers Count            : " << info.new_army_size << std::endl;
        out << "# Enemies Count             : " << info.new_enemy_size << std::endl;
        out << "# Available Ammo at Fronline: " << info.new_ammo_amount.back() << std::endl;
        out << "# Available Ammo at Rearline: " << info.new_ammo_amount.front() << std::endl;
        out << "#-------------------------------------------------" << std::endl;
        out << "# Battle Result: ";

        if(info.is_army_winning())
        {
            out << "YOU WIN!" << std::endl;
        }
        else
        {
            out << "YOU LOOSE!" << std::endl;
        }

        out << "#-------------------------------------------------" << std::endl;

        return out;
    }    
};

struct ModelOutput
{
    std::string model_output_filename;
    std::fstream output_file;
    const ModelInfo& model_info;

    ModelOutput(const std::string& prefix, const ModelInfo& _model_info, const ModelInputData& model_input)
        : model_output_filename(prefix + "_gentlemans_battle.dat"),
          model_info(_model_info)
    {
        output_file.open(model_output_filename, std::ios::out);
        output_file << model_input << std::endl << std::endl;
        output_file << "# Time  ArmySize    EnemySize" << std::endl;
    }

    void write_output_step()
    {
        output_file << std::setw(10) << std::setprecision(10) << std::fixed << std::setfill('0') <<
                       model_info.time << " " <<
                       model_info.old_army_size << " " <<
                       model_info.old_enemy_size << " " <<                       
                       model_info.old_ammo_amount.back() << " " <<
                       std::endl;
    }

    void stop_execution(bool b_show_gnuplot = true)
    {
        // Write Execution Summary into the output file
        output_file << model_info << std::endl;

        if(output_file.is_open())
        {
            output_file.close();
        }

        if(b_show_gnuplot)
        {
            std::string output_filename_quotes = "'" + model_output_filename + "'";
            std::string plot_command = "gnuplot -p -e \"plot " +
                    output_filename_quotes + " using 1:2 with lines title 'Soldiers Amount'," +
                    output_filename_quotes + " using 1:3 with lines title 'Enemies Amount'," +
                    output_filename_quotes + " using 1:4 with lines title 'Ammo at rear-guard'" +
                    "\"";
            std::cout << plot_command << std::endl;
            system(plot_command.data());
        }
    }
};

struct GentlesmanBattleModel
{
    ModelInputData input;
    ModelOutput output;
    ModelInfo info;
    int iteration = 0;

    typedef std::function<void()> ModelCallbackFunc;

    ModelCallbackFunc step_callback = nullptr;

    GentlesmanBattleModel() :
        output("", info, input), info(input)
    {
    }

    void run()
    {
        // Print parameters information
        std::cout << input << std::endl;

        do
        {
            output.write_output_step();
            info.advance_time();
            if(step_callback)
            {
                step_callback();
            }
            iteration++;
        }
        while(!info.should_stop());

        // Print execution summary
        std::cout << info << std::endl;
    }
};

// TODO: Organize this!
QCustomPlot* custom_plot = nullptr;

void init_plot()
{
    custom_plot = new QCustomPlot();

    // configure right and top axis to show ticks but no labels:
    // (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
    custom_plot->xAxis2->setVisible(true);
    custom_plot->xAxis2->setTickLabels(false);
    custom_plot->yAxis2->setVisible(true);
    custom_plot->yAxis2->setTickLabels(false);

    // make left and bottom axes always transfer their ranges to right and top axes:
    QObject::connect(custom_plot->xAxis, SIGNAL(rangeChanged(QCPRange)), custom_plot->xAxis2, SLOT(setRange(QCPRange)));
    QObject::connect(custom_plot->yAxis, SIGNAL(rangeChanged(QCPRange)), custom_plot->yAxis2, SLOT(setRange(QCPRange)));
}

void add_diffusion_plot(const GentlesmanBattleModel& model)
{
    // add two new graphs and set their look:
    static int graph_index = 0;
    custom_plot->addGraph();
    int graph_color = qBound(0, 255, 255 - graph_index * 30);
    custom_plot->graph(graph_index)->setPen(QPen(QColor(0, 0, graph_color)));

    // custom_plot->graph(graph_index)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue

    //
    // Convert model data to QCustomPlot format (QVector)
    //

    // Generate y-axis10
    QVector<double> y_axis = QVector<double>::fromStdVector(model.info.new_ammo_amount);

    // Generate x-axis    
    QVector<double> x_axis(y_axis.size());
    for(int i = 0; i < x_axis.size(); ++i)
    {
        x_axis[i] = i;
    }

    // pass data points to graphs:
    custom_plot->graph(graph_index)->setData(x_axis, y_axis);
    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
    custom_plot->graph(graph_index)->rescaleAxes();

    custom_plot->axisRect(0)->setRangeZoom(Qt::Vertical);

    graph_index++;
}

void show_plot()
{
    // Note: we could have also just called custom_plot->rescaleAxes(); instead
    // Allow user to drag axis rainfonges with mouse, zoom with mouse wheel and select graphs by clicking:
    custom_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    custom_plot->setGeometry(20, 20, 800, 800);
    custom_plot->show();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GentlesmanBattleModel model;

    // TODO: Organize this functions "init_plot(), add_diffusion_plot(), show_plot() in an object to handle this
    init_plot();

    model.step_callback = [&]()
    {
        static int num_plots = 0;
        if(num_plots < 1000)
        {
            if((model.iteration) % 100 == 0)
            {
                add_diffusion_plot(model);
                num_plots++;
            }
        }
    };

    show_plot();

    model.run();

    model.output.stop_execution();

    return app.exec();
}

