class PbbsRunsController < ApplicationController
  # GET /pbbs_runs
  # GET /pbbs_runs.xml
  def index
    @problem_id = params[:problem_id]
    @problem = PbbsProblem.find(:first, :conditions => ["id=?", @problem_id ])
    @orderby = "created desc"
    if (params[:orderby])
        @orderby = params[:orderby]
    end
    @pbbs_runs = PbbsRun.find(:all, :conditions => ["problem_id=?", @problem_id], :order => @orderby)

    @halloffame = []
    
    hosts = PbbsRun.connection.select_all("select distinct host_id from pbbs_runs r, pbbs_hosts h where problem_id=" + @problem_id + " and h.id=host_id order by h.numprocs desc")
    for hostid in hosts
      run = PbbsRun.connection.select_all("select id from pbbs_runs   where host_id=" + hostid["host_id"].to_s() + " and problem_id=" +@problem_id + "  order by mean_time asc limit 1")
      @halloffame = @halloffame + [PbbsRun.find(:first, :conditions => ["id=?", run[0]["id"] ])]
    end
    @halloffame = @halloffame.sort {|x,y| x.min_time <=> y.min_time }
    
    # Speedups
    @speedups = []
    extracond = ""
    if (params[:allprocs] == nil) 
      extracond = " and (r.numprocs mod 16 = 0 OR r.numprocs in (1,2,4,8,16))"
    end
    for h in hosts
        hostid = h["host_id"].to_s()
        programs = PbbsRun.connection.select_all("select distinct r.program_id, p.name, h.hostname from pbbs_runs r, pbbs_programs p, 
              pbbs_hosts h where r.host_id=h.id and r.program_id = p.id and r.problem_id=" +
                      @problem_id + " and host_id=" + hostid + " order by r.program_id")
        for p in programs
           spp = {"host" => p["hostname"], "program" => p["name"], "recs" => []}
           cpus = PbbsRun.connection.select_all("select distinct r.numprocs from pbbs_runs r, pbbs_programs p, pbbs_hosts h  
                        where r.host_id=h.id " + " and p.id=" + p["program_id"].to_s()+ " and r.program_id = p.id and r.problem_id=" + @problem_id + " and host_id=" + hostid +
                            " " + extracond + " order by r.numprocs asc")
           
           i=0
           for c in cpus
                i=i+1
                # Get last such run  (is there any better way?)
                r = PbbsRun.connection.select_all("select r.numprocs, r.median_time from pbbs_runs r, pbbs_programs p, pbbs_hosts h  
                              where r.host_id=h.id and r.program_id = p.id and r.problem_id=" + @problem_id + " and host_id=" + hostid + " and p.id=" + p["program_id"].to_s()+
                                  " and r.numprocs=" + c["numprocs"].to_s() + " order by r.id desc limit 1")
                r = r[0]
                if (i == 1) 
                  base = r["median_time"] * r["numprocs"]
                end
                spp["recs"] = spp["recs"] + [{"cpus" => r["numprocs"], "speedup" => base/r["median_time"], "runtime" => r["median_time"]}]      
            end
           @speedups = @speedups + [spp]
        end
    end  
    respond_to do |format|
      format.html # index.html.erb
      format.xml  { render :xml => @pbbs_runs }
    end
  end

  # GET /pbbs_runs/1
  # GET /pbbs_runs/1.xml
  def show
    @pbbs_run = PbbsRun.find(params[:id])

    respond_to do |format|
      format.html # show.html.erb
      format.xml  { render :xml => @pbbs_run }
    end
  end

  # GET /pbbs_runs/new
  # GET /pbbs_runs/new.xml
  def new
   
  end

  # GET /pbbs_runs/1/edit
  def edit
   end

  # POST /pbbs_runs
  # POST /pbbs_runs.xml
  def create
     
  end

  # PUT /pbbs_runs/1
  # PUT /pbbs_runs/1.xml
  def update
     
  end

  # DELETE /pbbs_runs/1
  # DELETE /pbbs_runs/1.xml
  def destroy
    @pbbs_run = PbbsRun.find(params[:id])
    @pbbs_run.destroy
    @problem_id = @pbbs_run.problem_id

    respond_to do |format|
      format.html { redirect_to(:controller => "pbbs_runs", :problem_id => @problem_id) }
      format.xml  { head :ok }
    end
  end
end
