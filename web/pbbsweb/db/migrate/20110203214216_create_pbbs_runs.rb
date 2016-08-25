class CreatePbbsRuns < ActiveRecord::Migration
  def self.up
    create_table :pbbs_runs do |t|

      t.timestamps
    end
  end

  def self.down
    drop_table :pbbs_runs
  end
end
