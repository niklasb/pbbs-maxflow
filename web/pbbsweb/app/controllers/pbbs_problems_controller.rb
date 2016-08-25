class PbbsProblemsController < ApplicationController
  # GET /pbbs_problems
  # GET /pbbs_problems.xml
  def index
    @pbbs_problems = PbbsProblem.all
    @pbbs_problems.each { |prob| prob[:recent] = get_recent_update(prob.id)}
    @pbbs_problems.each { |prob| prob[:recenttime] = time_since_last(prob.id)}
    
    if (params[:sort] == nil || params[:sort] == "recentrun")
      @pbbs_problems.sort! { |x,y| x.recenttime <=> y.recenttime } 
    elsif (params[:sort] == "name") 
      @pbbs_problems.sort! { |x,y| x.name <=> y.name } 
    end
    respond_to do |format|
      format.html # index.html.erb
      format.xml  { render :xml => @pbbs_problems }
    end
  end

  # GET /pbbs_problems/1
  # GET /pbbs_problems/1.xml
  def show
    @pbbs_problem = PbbsProblem.find(params[:id])

    respond_to do |format|
      format.html # show.html.erb
      format.xml  { render :xml => @pbbs_problem }
    end
  end
  
  def time_since_last(prbid)
    time = PbbsProblem.connection.select_all("select unix_timestamp(now())-unix_timestamp(max(r.created)) as mx from pbbs_runs r where problem_id=" + prbid.to_s())
    if (time[0]["mx"] != nil) 
      return time[0]["mx"]
    else 
      return 1e10
    end
  end
  
  def get_recent_update (prbid)
    time = PbbsProblem.connection.select_all("select to_days(now())-to_days(max(r.created)) as mx from pbbs_runs r where problem_id=" + prbid.to_s())
    
    if (time[0]["mx"] != nil) 
      days = time[0]["mx"]
      if (days == 0)
        return "today"
      end
      return days.to_s() + " day(s) ago"
    else 
      return ""
    end
  end

  # GET /pbbs_problems/new
  # GET /pbbs_problems/new.xml
  def new
    @pbbs_problem = PbbsProblem.new

    respond_to do |format|
      format.html # new.html.erb
      format.xml  { render :xml => @pbbs_problem }
    end
  end

  # GET /pbbs_problems/1/edit
  def edit
    @pbbs_problem = PbbsProblem.find(params[:id])
  end

  # POST /pbbs_problems
  # POST /pbbs_problems.xml
  def create
     
  end

  # PUT /pbbs_problems/1
  # PUT /pbbs_problems/1.xml
  def update
    @pbbs_problem = PbbsProblem.find(params[:id])

    respond_to do |format|
      if @pbbs_problem.update_attributes(params[:pbbs_problem])
        format.html { redirect_to(@pbbs_problem, :notice => 'Pbbs problem was successfully updated.') }
        format.xml  { head :ok }
      else
        format.html { render :action => "edit" }
        format.xml  { render :xml => @pbbs_problem.errors, :status => :unprocessable_entity }
      end
    end
  end

  # DELETE /pbbs_problems/1
  # DELETE /pbbs_problems/1.xml
  def destroy
     
  end
end
