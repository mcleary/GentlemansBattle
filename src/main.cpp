
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <limits>

#include <QtWidgets/QApplication>
#include "qcustomplot.h"

QCustomPlot* custom_plot;

struct ModelInputData
{
    double start_army_size;             // > 0          Tamanho inicial do exército
    double start_enemy_size;            // > 0          Tamanho inicial do exército inimigo
    double loose_battle_fraction;       // 0 < x < 1:   Porcentagem do exército que define derrota
    double army_skill;                  // 0 < x <= 1   Perícia do exército em eliminar inimigos
    double enemy_skill;                 // 0 < x <= 1   Perícia do inimigo em eliminar o exército
    double start_ammo;                  // > 0          Quantidade de munição que estará disponível durante a batalha
    double ammo_diffusion_coeffient;    // > 0          Velocidade com o que a munição é distribuída para o exército
    double formation_size;              // > 0          Tamanho da formação utilizada na batalha pelo exército
    double front_line_fraction;         // 0 < x <= 1   Porcentagem do exército que atuará na linha de frente

    double delta_time;                  // > 0
    double delta_x;                     // > 0

    /**
     * @brief ModelInputData default input data
     */
    ModelInputData()
    {
        start_army_size             = 100.0;
        start_enemy_size            = 100.0;
        loose_battle_fraction       = 0.01;
        army_skill                  = 0.001;
        enemy_skill                 = 0.001;
        start_ammo                  = 100;
        ammo_diffusion_coeffient    = 1.0;
        formation_size              = 10;
        front_line_fraction         = 0.1;

        delta_time                  = 0.1;
        delta_x                     = 0.7;

        std::cout << "Courant = " << ammo_diffusion_coeffient * delta_time / (delta_x * delta_x) << std::endl;
    }
};

struct ModelInfo
{
    double time             = 0.0;

    double new_army_size    = 0.0;
    double old_army_size    = 0.0;

    double new_enemy_size   = 0.0;
    double old_enemy_size   = 0.0;

    double diffusion_sum = 0;

    std::vector<double> new_ammo_amount;
    std::vector<double> old_ammo_amount;

    const ModelInputData& model_input;

    ModelInfo(const ModelInputData& input_data) :
        model_input(input_data)
    {
        // Setting up the mesh for the ammo diffusion
        new_ammo_amount.resize(input_data.formation_size / input_data.delta_x);
        old_ammo_amount.resize(new_ammo_amount.size());

        // Initial condition
        old_army_size = input_data.start_army_size;
        old_enemy_size = input_data.start_enemy_size;

        //old_ammo_amount[1] = model_input.start_ammo / model_input.delta_x;
        old_ammo_amount[10] = model_input.start_ammo / model_input.delta_x;
    }

    void advance_time(double delta_time)
    {
        double front_line_size = model_input.front_line_fraction * old_army_size;
        double enemy_front_line_size = model_input.front_line_fraction * old_enemy_size;

        // Army
        new_army_size = old_army_size - delta_time * model_input.enemy_skill * front_line_size * enemy_front_line_size;
        old_army_size = new_army_size;

        // Enemy
        double available_ammo = old_ammo_amount.back();
        new_enemy_size = old_enemy_size - delta_time * model_input.army_skill * available_ammo * front_line_size;
        old_enemy_size = new_enemy_size;

        // Ammo Diffusion
        const double ddt_dx2 = model_input.ammo_diffusion_coeffient * delta_time / (model_input.delta_x * model_input.delta_x);
        for(size_t i = 1; i < new_ammo_amount.size() - 1; ++i)
        {
            new_ammo_amount[i] = old_ammo_amount[i] + ddt_dx2 * (old_ammo_amount[i-1] - 2.0 * old_ammo_amount[i] + old_ammo_amount[i+1]);
        }
        // Ammo boundary conditions
        //        int i = 0;
        //        new_ammo_amount[i] = old_ammo_amount[i] + ddt_dx2 * (-2.0 * old_ammo_amount[i] + old_ammo_amount[i+1]);
        //        i = new_ammo_amount.size() - 1;
        //        new_ammo_amount.back() = old_ammo_amount[i] + ddt_dx2 * (old_ammo_amount[i-1] - 2.0 * old_ammo_amount[i]);

        new_ammo_amount[0] = new_ammo_amount[1];
        new_ammo_amount.back() = new_ammo_amount[new_ammo_amount.size() - 2];

        //        new_ammo_amount.front() = 0;
        //        new_ammo_amount.back() = 0;

        //new_ammo_amount.back() = *(old_ammo_amount.rbegin() + 1);
        //new_ammo_amount.back() -= old_ammo_amount.back() / (model_input.ammo_diffusion_coeffient * front_line_size);
        new_ammo_amount.swap(old_ammo_amount);

        // Debug Code
        diffusion_sum = 0;
        for(auto diff : new_ammo_amount)
        {
            diffusion_sum += diff;
        }

        time += delta_time;
    }

    bool should_stop() const
    {
        return new_army_size <= model_input.start_army_size * model_input.loose_battle_fraction ||
                new_enemy_size <= model_input.start_enemy_size * model_input.loose_battle_fraction;
    }
};

std::ostream& operator<< (std::ostream& out, const ModelInputData& input_data);

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

        if(!output_file.is_open())
        {
            throw std::runtime_error("Cannot open outputfile: " + model_output_filename);;
        }

        output_file << model_input << std::endl << std::endl;
        output_file << "# Time  ArmySize    EnemySize" << std::endl;
    }

    void write()
    {


        output_file << std::setw(10) << std::setprecision(10) << std::fixed << std::setfill('0') <<
                       model_info.time << " " <<
                       model_info.old_army_size << " " <<
                       model_info.old_enemy_size << " " <<
                       model_info.old_ammo_amount[model_info.old_ammo_amount.size() - 1] << " " <<
                       model_info.old_ammo_amount[0] << " " <<
                       model_info.diffusion_sum <<

                       std::endl;
    }

    void show_plot()
    {
        if(output_file.is_open())
        {
            output_file.close();
        }

        std::string output_filename_quotes = "'" + model_output_filename + "'";
        std::string plot_command = "gnuplot -p -e \"plot " +
                output_filename_quotes + " using 1:2 with lines title 'Soldiers Amount'," +
                output_filename_quotes + " using 1:3 with lines title 'Enemies Amount'," +
                output_filename_quotes + " using 1:4 with lines title 'Ammo at front-line'," +
                output_filename_quotes + " using 1:5 with lines title 'Ammo at rear-guard'" +
                "\"";
        std::cout << plot_command << std::endl;
        system(plot_command.data());
    }
};


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ModelInputData model_input;
    ModelInfo model_info(model_input);
    ModelOutput model_output("", model_info, model_input);

    do
    {
        model_output.write();
        model_info.advance_time(model_input.delta_time);
    }
    while(!model_info.should_stop());

    model_output.show_plot();

    //return EXIT_SUCCESS;


    custom_plot = new QCustomPlot();
    // add two new graphs and set their look:
    custom_plot->addGraph();
    custom_plot->graph(0)->setPen(QPen(Qt::blue)); // line color blue for first graph
    custom_plot->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue
    // generate some points of data (y0 for first, y1 for second graph):
    QVector<double> x(250), y0(250), y1(250);
    for (int i=0; i<250; ++i)
    {
        x[i] = i;
        y0[i] = qExp(-i/150.0)*qCos(i/10.0); // exponentially decaying cosine
        y1[i] = qExp(-i/150.0);              // exponential envelope
    }
    // configure right and top axis to show ticks but no labels:
//     (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
    custom_plot->xAxis2->setVisible(true);
    custom_plot->xAxis2->setTickLabels(false);
    custom_plot->yAxis2->setVisible(true);
    custom_plot->yAxis2->setTickLabels(false);
    // make left and bottom axes always transfer their ranges to right and top axes:
    QObject::connect(custom_plot->xAxis, SIGNAL(rangeChanged(QCPRange)), custom_plot->xAxis2, SLOT(setRange(QCPRange)));
    QObject::connect(custom_plot->yAxis, SIGNAL(rangeChanged(QCPRange)), custom_plot->yAxis2, SLOT(setRange(QCPRange)));
    // pass data points to graphs:
//    QVector<double> data()
    custom_plot->graph(0)->setData(x, y0);
    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
    custom_plot->graph(0)->rescaleAxes();
    // same thing for graph 1, but only enlarge ranges (in case graph 1 is smaller than graph 0):
    // Note: we could have also just called custom_plot->rescaleAxes(); instead
    // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
    custom_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);


    custom_plot->setGeometry(20, 20, 800, 800);
    custom_plot->show();

    return app.exec();
}


std::ostream& operator<< (std::ostream& out, const ModelInputData& input_data)
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

